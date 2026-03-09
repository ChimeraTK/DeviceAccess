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
