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

BOOST_AUTO_TEST_CASE(test_read_buffer0) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  auto backend = boost::dynamic_pointer_cast<NumericAddressedBackend>(device.getBackend());
  BOOST_REQUIRE(backend);

  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();

  auto registerInfo = backend->getRegisterInfo("DAQ.FD");
  BOOST_REQUIRE(registerInfo.doubleBuffer != std::nullopt);

  auto dbInfo = registerInfo.doubleBuffer.value();

  TestableDoubleBufferAccessor<int> accessor(
      dbInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  /* firmware-visible buffers */
  auto buf0 = device.getTwoDRegisterAccessor<int>("/DAQ/FD/BUF0");
  auto buf1 = device.getTwoDRegisterAccessor<int>("/DAQ/FD/BUF1");

  auto inactive = device.getOneDRegisterAccessor<int>("/DAQ/DOUBLE_BUF/INACTIVE_BUF_ID");

  /* simulate firmware writing to buffer0 */
  buf0[0][0] = 4;
  buf0[0][1] = 8;
  buf0[0][2] = 12;
  buf0[0][3] = 16;
  buf0.write();

  /* firmware state:
     inactive = 1 → firmware writes BUF0
  */

  inactive[0] = 1;
  inactive.write();

  // enable[1] = 1;
  // enable.write();

  accessor.doPreRead(TransferType::read);
  accessor.doReadTransferSynchronously();
  accessor.doPostRead(TransferType::read, true);

  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][0], 4);
  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][1], 8);
  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][2], 12);
  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][3], 16);

  inactive[0] = 0;
  inactive.write();

  buf1[0][0] = 140;
  buf1[0][1] = 144;
  buf1[0][2] = 148;
  buf1[0][3] = 152;
  buf1.write();

  accessor.doPreRead(TransferType::read);
  accessor.doReadTransferSynchronously();
  accessor.doPostRead(TransferType::read, true);

  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][0], 140);
  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][1], 144);
  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][2], 148);
  BOOST_CHECK_EQUAL(accessor.buffer_2D[0][3], 152);
}

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

  std::atomic<bool> secondEntered{false};

  /* Thread 1 acquires transfer lock */
  accessor1.doPreRead(TransferType::read);

  /* start accessor2 asynchronously */
  auto future = std::async(std::launch::async, [&] {
    accessor2.doPreRead(TransferType::read);
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

BOOST_AUTO_TEST_CASE(test_mutex_usecount) {
  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();

  mutex->lock();
  BOOST_CHECK_EQUAL(mutex->useCount(), 1);

  mutex->lock();
  BOOST_CHECK_EQUAL(mutex->useCount(), 2);

  mutex->unlock();
  BOOST_CHECK_EQUAL(mutex->useCount(), 1);

  mutex->unlock();
  BOOST_CHECK_EQUAL(mutex->useCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_exception_does_not_leave_lock) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");

  auto backend = boost::dynamic_pointer_cast<NumericAddressedBackend>(device.getBackend());
  BOOST_REQUIRE(backend);

  auto mutex = std::make_shared<detail::CountedRecursiveMutex>();

  auto registerInfo = backend->getRegisterInfo("DAQ.FD");
  auto dbInfo = registerInfo.doubleBuffer.value();

  TestableDoubleBufferAccessor<int> accessor(
      dbInfo, backend, mutex, RegisterPath("/DAQ/FD"), 16384, 0, AccessModeFlags{});

  // Acquire lock via pre-read
  accessor.doPreRead(TransferType::read);
  BOOST_CHECK_EQUAL(mutex->useCount(), 1); // lock acquired

  // Simulate exception during transfer
  bool exceptionThrown = false;
  try {
    throw std::runtime_error("simulated transfer exception");
  }
  catch(const std::runtime_error&) {
    exceptionThrown = true;
    // mandate: still call postRead to release lock
    accessor.doPostRead(TransferType::read, false);
  }

  BOOST_CHECK(exceptionThrown);
  BOOST_CHECK_EQUAL(mutex->useCount(), 0); // lock released
}

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

  // Get the actual register for handshake
  auto enableReg = device.getOneDRegisterAccessor<uint32_t>("/DAQ/DOUBLE_BUF/ENA");
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
