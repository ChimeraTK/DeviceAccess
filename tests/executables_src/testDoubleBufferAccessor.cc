// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE testDoubleBufferAccessor
#include "Device.h"
#include "DoubleBufferAccessor.h"
#include "DummyBackend.h"

#include <boost/test/unit_test.hpp>

using namespace ChimeraTK;

namespace {

  /* expose protected members */
  template<typename T>
  class TestableDoubleBufferAccessor : public DoubleBufferAccessor<T> {
   public:
    using DoubleBufferAccessor<T>::DoubleBufferAccessor;
    using DoubleBufferAccessor<T>::buffer_2D;
  };
} // namespace

// ------------------------------------------------------------
// Test that _enableDoubleBufferReg toggles during pre/post read
BOOST_AUTO_TEST_CASE(test_firmware_handshake_toggle) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");
  auto backend = boost::dynamic_pointer_cast<NumericAddressedBackend>(device.getBackend());
  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();
  auto registerInfo = backend->getRegisterInfo("DAQ.FD");
  auto dbInfo = registerInfo.doubleBuffer.value();

  TestableDoubleBufferAccessor<int> accessor(
      dbInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  // Get the actual register for handshake (index 1, matching DAQ.FD's double buffer config)
  auto enableReg = device.getOneDRegisterAccessor<uint32_t>("/DAQ/DOUBLE_BUF/ENA", 1, 1);
  enableReg.read();
  BOOST_CHECK_EQUAL(enableReg[0], 1); // must be enabled*/

  // Pre-read should disable the double buffer
  accessor.doPreRead(TransferType::read);
  enableReg.read();
  BOOST_CHECK_EQUAL(enableReg[0], 0); // must be disabled

  // Read transfer
  accessor.doReadTransferSynchronously();

  // Post-read should re-enable the double buffer
  accessor.doPostRead(TransferType::read, true);
  enableReg.read();
  BOOST_CHECK_EQUAL(enableReg[0], 1); // must be re-enabled*/
}

// ------------------------------------------------------------
// Test the transfer lock blocks other accessors using the same handshake
BOOST_AUTO_TEST_CASE(test_transfer_lock_blocks_other_accessor) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  auto backend = boost::dynamic_pointer_cast<NumericAddressedBackend>(device.getBackend());
  BOOST_REQUIRE(backend);

  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();

  auto registerInfo = backend->getRegisterInfo("DAQ.FD");
  auto dbInfo = registerInfo.doubleBuffer.value();

  TestableDoubleBufferAccessor<int> accessor1(
      dbInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  TestableDoubleBufferAccessor<int> accessor2(
      dbInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  /* Thread 1 acquires transfer lock */
  accessor1.doPreRead(TransferType::read);

  /* start accessor2 asynchronously */
  auto future = std::async(std::launch::async, [&] {
    accessor2.doPreRead(TransferType::read);
    accessor2.doReadTransferSynchronously();
    accessor2.doPostRead(TransferType::read, false);
  });

  /* accessor2 must still be blocked */
  auto status = future.wait_for(std::chrono::milliseconds(50));
  BOOST_CHECK(status == std::future_status::timeout);

  /* release lock */
  accessor1.doPostRead(TransferType::read, false);

  /* now accessor2 must complete */
  status = future.wait_for(std::chrono::milliseconds(200));
  BOOST_CHECK(status == std::future_status::ready);
}

// ------------------------------------------------------------
// Test two registers sharing the same double buffering handshake
// (e.g. DAQ.FD and DAQ.MACRO_PULSE_NUMBER both use DAQ.DOUBLE_BUF.ENA index 1)
BOOST_AUTO_TEST_CASE(test_shared_handshake_two_registers) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  auto backend = boost::dynamic_pointer_cast<NumericAddressedBackend>(device.getBackend());
  BOOST_REQUIRE(backend);

  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();

  auto fdInfo = backend->getRegisterInfo("DAQ.FD").doubleBuffer.value();
  auto mpnInfo = backend->getRegisterInfo("DAQ.MACRO_PULSE_NUMBER").doubleBuffer.value();

  TestableDoubleBufferAccessor<int> fdAccessor(
      fdInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  TestableDoubleBufferAccessor<int> mpnAccessor(
      mpnInfo, backend, mutex, RegisterPath("/DAQ/MACRO_PULSE_NUMBER"), 1, 0, AccessModeFlags{});

  // Both accessors share the same mutex and the same ENA/INACTIVE_BUF_ID registers (same index)
  // When one holds the transfer lock, the other should block

  fdAccessor.doPreRead(TransferType::read);

  auto future = std::async(std::launch::async, [&] {
    mpnAccessor.doPreRead(TransferType::read);
    mpnAccessor.doReadTransferSynchronously();
    mpnAccessor.doPostRead(TransferType::read, false);
  });

  auto status = future.wait_for(std::chrono::milliseconds(50));
  BOOST_CHECK(status == std::future_status::timeout);

  fdAccessor.doPostRead(TransferType::read, false);

  status = future.wait_for(std::chrono::milliseconds(200));
  BOOST_CHECK(status == std::future_status::ready);
}

// ------------------------------------------------------------
// Test buffer selection when INACTIVE_BUF_ID reports buffer 1
// Exercises the _currentBuffer==1 branches in pre/read/post
BOOST_AUTO_TEST_CASE(test_buffer_selection_current_buffer_1) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");
  auto backend = boost::dynamic_pointer_cast<NumericAddressedBackend>(device.getBackend());

  // Write INACTIVE_BUF_ID[index=1] = 1 via the public device API
  auto inactiveBuf = device.getOneDRegisterAccessor<uint32_t>("/DAQ/DOUBLE_BUF/INACTIVE_BUF_ID", 1, 1);
  inactiveBuf[0] = 1;
  inactiveBuf.write();

  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();
  auto registerInfo = backend->getRegisterInfo("DAQ.FD");
  auto dbInfo = registerInfo.doubleBuffer.value();

  TestableDoubleBufferAccessor<int> accessor(
      dbInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  // Do a full read cycle — _currentBuffer should be 1
  accessor.doPreRead(TransferType::read);
  accessor.doReadTransferSynchronously();
  // Must not crash — verifies _currentBuffer==1 path works
  accessor.doPostRead(TransferType::read, false);
}
