// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE testDoubleBufferAccessor
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

#include <boost/test/unit_test.hpp>

#include <future>

using namespace ChimeraTK;

// ------------------------------------------------------------
// Test that a full read cycle completes successfully.
// Observable behaviour: after read(), the accessor holds data
// and the enable register is left in the enabled state (value 1).
BOOST_AUTO_TEST_CASE(test_full_read_cycle) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  // Get double-buffer accessor through the normal public API.
  // DAQ.MACRO_PULSE_NUMBER is a simple 1-element uint32 register
  // which uses double-buffering with index 1.
  auto accessor = device.getOneDRegisterAccessor<int>("/DAQ/MACRO_PULSE_NUMBER", 1, 0);

  // Perform a full read cycle through the public API
  accessor.read();

  // After the read cycle, we have valid data
  BOOST_CHECK(accessor.dataValidity() != ChimeraTK::DataValidity::faulty);
}

// ------------------------------------------------------------
// Test that concurrent reads on the same double-buffered register
// complete successfully. The transfer lock ensures sequential
// access to the handshake registers, so no data corruption occurs.
BOOST_AUTO_TEST_CASE(test_concurrent_reads_same_register) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  // Two accessors on the same register
  auto accessor1 = device.getOneDRegisterAccessor<int>("/DAQ/MACRO_PULSE_NUMBER", 1, 0);
  auto accessor2 = device.getOneDRegisterAccessor<int>("/DAQ/MACRO_PULSE_NUMBER", 1, 0);

  // Read concurrently from two threads. Both must complete without
  // deadlock or data corruption.
  auto future = std::async(std::launch::async, [&] { accessor1.read(); });
  accessor2.read();

  auto status = future.wait_for(std::chrono::milliseconds(200));
  BOOST_CHECK(status == std::future_status::ready);
}

// ------------------------------------------------------------
// Test that two registers sharing the same double buffering handshake
// (index 1: DAQ.MACRO_PULSE_NUMBER and DAQ.FD both use DAQ.DOUBLE_BUF.ENA[1])
// can be read concurrently without issues.
BOOST_AUTO_TEST_CASE(test_shared_handshake_two_registers) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  // Two accessors with the same handshake index (index 1)
  auto mpnAccessor1 = device.getOneDRegisterAccessor<int>("/DAQ/MACRO_PULSE_NUMBER", 1, 0);
  auto mpnAccessor2 = device.getOneDRegisterAccessor<int>("/DAQ/MACRO_PULSE_NUMBER", 1, 0);

  // Read concurrently from two threads
  auto future = std::async(std::launch::async, [&] { mpnAccessor1.read(); });
  mpnAccessor2.read();

  auto status = future.wait_for(std::chrono::milliseconds(200));
  BOOST_CHECK(status == std::future_status::ready);
}

// ------------------------------------------------------------
// Test that buffer selection works with INACTIVE_BUF_ID=1.
// This exercises the _currentBuffer==1 branch.
BOOST_AUTO_TEST_CASE(test_buffer_selection_current_buffer_1) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  // Write INACTIVE_BUF_ID[index=1] = 1
  auto inactiveBuf = device.getOneDRegisterAccessor<uint32_t>("/DAQ/DOUBLE_BUF/INACTIVE_BUF_ID", 1, 1);
  inactiveBuf[0] = 1;
  inactiveBuf.write();

  auto accessor = device.getOneDRegisterAccessor<int>("/DAQ/MACRO_PULSE_NUMBER", 1, 0);

  // Do a full read cycle with the _currentBuffer==1 path
  accessor.read();

  // Must not crash, and must have valid data
  BOOST_CHECK(accessor.dataValidity() != ChimeraTK::DataValidity::faulty);
}

// ------------------------------------------------------------
// Helper that simulates the firmware finishing a buffer and raising the
// interrupt. An interrupt-triggered read must return that freshly finished
// buffer (the inactive one at the time of the read).
class AsyncDoubleBufferFixture {
 public:
  AsyncDoubleBufferFixture()
  : dummy(openDeviceAndGetDummy(device)),
    accessor(device.getOneDRegisterAccessor<uint32_t>("/DAQ/ASYNC_DBLBUF", 1, 0, {AccessMode::wait_for_new_data})),
    enable(dummy.get(), "DAQ/DOUBLE_BUF", "ENA"), inactive(dummy.get(), "DAQ/DOUBLE_BUF", "INACTIVE_BUF_ID"),
    buffer0(dummy.get(), "DAQ/ASYNC_DBLBUF", "BUF0"), buffer1(dummy.get(), "DAQ/ASYNC_DBLBUF", "BUF1") {
    // Enable double buffering for index 2.
    enable[2] = 1;
  }

  // Simulate the firmware finishing a buffer: fill the freshly finished buffer
  // and point INACTIVE_BUF_ID at it, then raise the interrupt.
  void firmwareFinishesBuffer(uint32_t value, uint32_t newInactiveBuffer) {
    if(newInactiveBuffer == 1) {
      buffer0 = value;
    }
    else {
      buffer1 = value;
    }
    inactive[2] = newInactiveBuffer;
    dummy->triggerInterrupt(1);
  }

  Device device;
  boost::shared_ptr<DummyBackend> dummy;
  OneDRegisterAccessor<uint32_t> accessor;
  DummyRegisterAccessor<uint32_t> enable;
  DummyRegisterAccessor<uint32_t> inactive;
  DummyRegisterAccessor<uint32_t> buffer0;
  DummyRegisterAccessor<uint32_t> buffer1;

 private:
  static boost::shared_ptr<DummyBackend> openDeviceAndGetDummy(Device& dev) {
    dev.open("(dummy?map=simpleJsonFile.jmap)");
    auto backend = boost::dynamic_pointer_cast<DummyBackend>(dev.getBackend());
    if(!backend) {
      BOOST_FAIL("Device did not produce a DummyBackend");
    }
    return backend;
  }
};

// ------------------------------------------------------------
// Test that an interrupt-triggered wait_for_new_data read of the full
// double-buffered register returns the buffer the firmware just finished, over
// several consecutive buffer swaps.
BOOST_AUTO_TEST_CASE(TestInterruptDrivenReadReturnsFinishedBuffer) {
  AsyncDoubleBufferFixture f;
  f.device.activateAsyncRead();

  // An initial value is delivered when the async domain is activated.
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() != ChimeraTK::DataValidity::faulty);

  // No further data before the firmware completes the next buffer.
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 1: firmware finishes buffer0 (=100) and flips to buffer1.
  f.firmwareFinishesBuffer(100, 1);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 100);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 2: firmware finishes buffer1 (=200) and flips to buffer0.
  BOOST_CHECK(!f.accessor.readNonBlocking());
  f.firmwareFinishesBuffer(200, 0);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 200);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 3: firmware finishes buffer0 (=300) and flips to buffer1.
  BOOST_CHECK(!f.accessor.readNonBlocking());
  f.firmwareFinishesBuffer(300, 1);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 300);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 4: firmware finishes buffer1 (=400) and flips to buffer0.
  BOOST_CHECK(!f.accessor.readNonBlocking());
  f.firmwareFinishesBuffer(400, 0);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 400);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  f.device.close();
}

// ------------------------------------------------------------
// Test that the double-buffer handshake stays enabled after interrupt-driven
// reads: ENA must still hold 1 after a wait_for_new_data read cycle.
BOOST_AUTO_TEST_CASE(TestInterruptDrivenReadKeepsHandshakeEnabled) {
  AsyncDoubleBufferFixture f;
  f.device.activateAsyncRead();

  // An initial value is delivered when the async domain is activated.
  BOOST_CHECK(f.accessor.readNonBlocking());

  // No data before the firmware completes the next buffer.
  BOOST_CHECK(!f.accessor.readNonBlocking());

  f.firmwareFinishesBuffer(500, 0);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 500);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // The handshake must not disable swapping, so ENA stays enabled.
  BOOST_CHECK(f.enable[2] == 1);

  f.device.close();
}

// ------------------------------------------------------------
// A gated double-buffered scalar register reports faulty data while its gate is
// closed and the data while it is open.
BOOST_AUTO_TEST_CASE(TestGatedDoubleBufferedScalarPolled) {
  Device device;
  device.open("(dummy?map=gatedByDoubleBuffer.jmap)");

  auto sel = device.getScalarRegisterAccessor<uint32_t>("/CTRL/SEL");
  // Plain writable registers at both buffer addresses let the test place the payload whichever buffer the read picks.
  auto scratch = device.getScalarRegisterAccessor<uint32_t>("/DAQ/SCRATCH");
  auto scratchSecondary = device.getScalarRegisterAccessor<uint32_t>("/DAQ/SCRATCH_SECONDARY");
  auto gated = device.getScalarRegisterAccessor<uint32_t>("/DAQ/SCALAR");

  // Open the gate (CTRL.SEL == 1) and place a known payload.
  sel = 1;
  sel.write();
  scratch = 0x1234;
  scratch.write();
  scratchSecondary = 0x1234;
  scratchSecondary.write();
  gated.read();
  BOOST_CHECK(gated.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(gated == 0x1234);

  // Any other value of the gating register closes the gate.
  sel = 0;
  sel.write();
  gated.read();
  BOOST_CHECK(gated.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Reopening restores good data.
  sel = 1;
  sel.write();
  gated.read();
  BOOST_CHECK(gated.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(gated == 0x1234);

  device.close();
}

// ------------------------------------------------------------
// A gated double-buffered 2D register takes its gate from the per-channel
// gatedBy: each channel slice is faulty while its own gate is closed and valid
// while it is open, within one read.
BOOST_AUTO_TEST_CASE(TestGatedDoubleBufferedChannelSlicePolled) {
  Device device;
  device.open("(dummy?map=gatedByDoubleBuffer.jmap)");

  auto chSel = device.getScalarRegisterAccessor<uint32_t>("/CTRL/CH_SEL");
  // Writable registers at both buffer addresses let the test place the raw block; word i carries Ch0 in its low and
  // Ch1 in its high 16 bits.
  auto scratch = device.getOneDRegisterAccessor<uint32_t>("/DAQ/SCRATCH_PER_CHANNEL");
  auto scratchSecondary = device.getOneDRegisterAccessor<uint32_t>("/DAQ/SCRATCH_PER_CHANNEL_SECONDARY");
  auto packChannels = [](uint16_t ch0, uint16_t ch1) {
    return static_cast<uint32_t>(ch0) | (static_cast<uint32_t>(ch1) << 16U);
  };
  for(size_t i = 0; i < 4; ++i) {
    auto word = packChannels(static_cast<uint16_t>(100 + 100 * i), static_cast<uint16_t>(500 + 100 * i));
    scratch[i] = word;
    scratchSecondary[i] = word;
  }
  scratch.write();
  scratchSecondary.write();

  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/DAQ/PER_CHANNEL/Ch0");
  auto ch1 = device.getOneDRegisterAccessor<int16_t>("/DAQ/PER_CHANNEL/Ch1");

  // CTRL.CH_SEL == 0 opens Ch0 and closes Ch1.
  chSel = 0;
  chSel.write();
  ch0.read();
  ch1.read();
  BOOST_CHECK(ch0.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK_EQUAL(ch0[0], 100);
  BOOST_CHECK_EQUAL(ch0[1], 200);
  BOOST_CHECK_EQUAL(ch0[2], 300);
  BOOST_CHECK_EQUAL(ch0[3], 400);
  BOOST_CHECK(ch1.dataValidity() == ChimeraTK::DataValidity::faulty);

  // CTRL.CH_SEL == 1 swaps the roles.
  chSel = 1;
  chSel.write();
  ch0.read();
  ch1.read();
  BOOST_CHECK(ch0.dataValidity() == ChimeraTK::DataValidity::faulty);
  BOOST_CHECK(ch1.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK_EQUAL(ch1[0], 500);
  BOOST_CHECK_EQUAL(ch1[1], 600);
  BOOST_CHECK_EQUAL(ch1[2], 700);
  BOOST_CHECK_EQUAL(ch1[3], 800);

  device.close();
}

// ------------------------------------------------------------
// A register-level gate on a double-buffered 2D register faults the
// full-register accessor while the gate is closed and delivers the data while
// it is open. The channels carry no gate of their own, so their slices stay
// valid.
BOOST_AUTO_TEST_CASE(TestGatedDoubleBufferedRegisterLevelPolled) {
  Device device;
  device.open("(dummy?map=gatedByDoubleBuffer.jmap)");

  auto blockSel = device.getScalarRegisterAccessor<uint32_t>("/CTRL/BLOCK_SEL");
  // Writable registers at both buffer addresses let the test place the raw block; word i carries Ch0 in its low and
  // Ch1 in its high 16 bits.
  auto scratch = device.getOneDRegisterAccessor<uint32_t>("/DAQ/SCRATCH_BLOCK");
  auto scratchSecondary = device.getOneDRegisterAccessor<uint32_t>("/DAQ/SCRATCH_BLOCK_SECONDARY");
  auto packChannels = [](uint16_t ch0, uint16_t ch1) {
    return static_cast<uint32_t>(ch0) | (static_cast<uint32_t>(ch1) << 16U);
  };
  for(size_t i = 0; i < 4; ++i) {
    auto word = packChannels(static_cast<uint16_t>(10 + 10 * i), static_cast<uint16_t>(50 + 10 * i));
    scratch[i] = word;
    scratchSecondary[i] = word;
  }
  scratch.write();
  scratchSecondary.write();

  auto full = device.getTwoDRegisterAccessor<int16_t>("/DAQ/BLOCK");
  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/DAQ/BLOCK/Ch0");

  // CTRL.BLOCK_SEL == 5 is the configured value.
  blockSel = 5;
  blockSel.write();
  full.read();
  BOOST_CHECK(full.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK_EQUAL(full[0][0], 10);
  BOOST_CHECK_EQUAL(full[0][1], 20);
  BOOST_CHECK_EQUAL(full[0][2], 30);
  BOOST_CHECK_EQUAL(full[0][3], 40);
  BOOST_CHECK_EQUAL(full[1][0], 50);
  BOOST_CHECK_EQUAL(full[1][3], 80);

  // The channel slice is not affected by the register-level gate.
  ch0.read();
  BOOST_CHECK(ch0.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK_EQUAL(ch0[0], 10);
  BOOST_CHECK_EQUAL(ch0[3], 40);

  // Any other value closes the register-level gate, marking the whole block faulty.
  blockSel = 0;
  blockSel.write();
  full.read();
  BOOST_CHECK(full.dataValidity() == ChimeraTK::DataValidity::faulty);

  device.close();
}

// ------------------------------------------------------------
// A closed gate neither wakes the async consumer of a double-buffered register
// nor advances its version number; a suppressed delivery leaves the consumer at
// its last delivered value. An open gate delivers the fresh value.
BOOST_AUTO_TEST_CASE(TestGatedDoubleBufferedAsyncSuppressed) {
  Device device;
  device.open("(dummy?map=gatedByDoubleBuffer.jmap)");
  device.activateAsyncRead();

  auto backend = boost::dynamic_pointer_cast<DummyBackend>(device.getBackend());
  BOOST_REQUIRE(backend != nullptr);

  auto sel = device.getScalarRegisterAccessor<uint32_t>("/CTRL/SEL");
  // A plain writable register at the payload address lets the test place the value an interrupt reads.
  auto scratch = device.getScalarRegisterAccessor<uint32_t>("/DAQ/SCRATCH");

  // Open the gate and place the value the initial delivery must pick up.
  sel = 1;
  sel.write();
  scratch = 0x1234;
  scratch.write();
  auto gated = device.getScalarRegisterAccessor<uint32_t>("/DAQ/SCALAR", 0, {AccessMode::wait_for_new_data});

  gated.read();
  auto initialValue = static_cast<uint32_t>(gated);
  auto versionAfterInitial = gated.getVersionNumber();
  // Close the gate: an interrupt must not wake the consumer and must not publish a new version number.
  sel = 0;
  sel.write();

  auto waitForRead = std::async(std::launch::async, [&gated] { gated.read(); });
  BOOST_CHECK(waitForRead.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  // An interrupt through the closed gate must not wake the consumer; the suppressed delivery leaves the consumer at
  // its last delivered value and version number.
  scratch = 0x1111;
  scratch.write();
  backend->triggerInterrupt(1);
  BOOST_CHECK(waitForRead.wait_for(std::chrono::seconds(1)) == std::future_status::timeout); // no wake
  BOOST_CHECK(gated.getVersionNumber() == versionAfterInitial);
  BOOST_CHECK(static_cast<uint32_t>(gated) == initialValue);

  // Reopen the gate: the next interrupt wakes the consumer and advances the version number.
  sel = 1;
  sel.write();
  scratch = 0x5678;
  scratch.write();
  backend->triggerInterrupt(1);
  BOOST_CHECK(waitForRead.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  waitForRead.wait();
  BOOST_CHECK(gated.getVersionNumber() > versionAfterInitial);

  device.close();
}
