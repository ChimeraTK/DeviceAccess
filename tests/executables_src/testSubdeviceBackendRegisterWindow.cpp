// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

#include <boost/thread/barrier.hpp>

#include <thread>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendTest
#define BOOST_NO_EXCEPTIONS
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;
#undef BOOST_NO_EXCEPTIONS

using namespace ChimeraTK;

#define CHECK_TIMEOUT(execPreCheck, condition, maxMilliseconds)                                                        \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    execPreCheck while(!(condition)) {                                                                                 \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) break;                                                                                       \
      usleep(1000);                                                                                                    \
      execPreCheck                                                                                                     \
    }                                                                                                                  \
  }

BOOST_AUTO_TEST_SUITE(SubdeviceBackendTestSuite)

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test3regsScalar) {
  setDMapFilePath("subdeviceTest.dmap");

  Device dev;
  dev.open("SUBDEV_REG_WINDOW_3REG_MODE");
  Device target;
  target.open("TARGET1");

  auto acc1 = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER1");
  auto acc2 = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER2");
  auto accA = target.getScalarRegisterAccessor<int32_t>("APP.1.ADDRESS");
  auto accD = target.getScalarRegisterAccessor<int32_t>("APP.1.DATA");
  auto accS = target.getScalarRegisterAccessor<int32_t>("APP.1.STATUS");
  std::atomic<bool> done;
  std::thread t;

  BOOST_CHECK_THROW(acc1.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(acc2.read(), ChimeraTK::logic_error);

  accS = 1;
  accS.write();
  done = false;
  t = std::thread([&] {
    acc2 = 42;
    acc2.write();
    done = true;
  });
  usleep(10000);
  BOOST_CHECK(done == false);
  accS = 0;
  accS.write();
  CHECK_TIMEOUT(accA.read();, (accA == 1), 5000);
  t.join();
  accD.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(accD), 42 * 4);

  acc1 = 120;
  acc1.write();
  accA.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(accA), 0);
  accD.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(accD), 120);

  dev.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test3regsArray) {
  setDMapFilePath("subdeviceTest.dmap");

  Device dev;
  dev.open("SUBDEV_REG_WINDOW_3REG_MODE");
  Device target;
  target.open("TARGET1");

  auto accArea = dev.getOneDRegisterAccessor<double>("APP.0.MY_AREA2");
  auto accA = target.getScalarRegisterAccessor<int32_t>("APP.1.ADDRESS");
  auto accD = target.getScalarRegisterAccessor<int32_t>("APP.1.DATA");
  auto accS = target.getScalarRegisterAccessor<int32_t>("APP.1.STATUS");
  std::atomic<bool> done;
  std::thread t;

  accS = 1;
  accS.write();
  done = false;
  t = std::thread([&] {
    accArea[0] = 123;
    accArea[1] = 456;
    accArea.write();
    done = true;
  });
  usleep(10000);
  BOOST_CHECK(done == false);
  accS = 0;
  accS.write();
  CHECK_TIMEOUT(accA.read();, (accA == 9), 5000);
  t.join();
  accD.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(accD), 456);

  /// @todo Make a proper test with a custom backend, to make sure all elements
  /// of the array are properly written

  dev.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test3regsByteOffset1) {
  setDMapFilePath("subdeviceTest.dmap");

  Device dev;
  dev.open("SUBDEV_REG_WINDOW_3REG_MODE");
  Device target;
  target.open("TARGET1");

  auto acc = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER_AT_BYTE_1");
  auto accA = target.getScalarRegisterAccessor<uint32_t>("APP.1.ADDRESS");
  auto accD = target.getScalarRegisterAccessor<uint32_t>("APP.1.DATA");
  auto accS = target.getScalarRegisterAccessor<uint32_t>("APP.1.STATUS");
  std::atomic<bool> done;
  std::thread t;

  BOOST_CHECK_THROW(acc.read(), ChimeraTK::logic_error);

  accS = 1;
  accS.write();
  done = false;
  t = std::thread([&] {
    acc = std::bit_cast<int32_t>(0xdeadbeef);
    acc.write();
    done = true;
  });
  usleep(10000);
  BOOST_CHECK(done == false);
  accS = 0;
  accS.write();
  t.join();
  accA.read();
  // There have been two transfers, the last one to trasfer address 1.
  // This is not the byte 1 in the map file!
  BOOST_CHECK(accA == 1);
  accD.read();
  // Leading/trailing bytes are padded with 0s. We can just see the last word.
  BOOST_TEST(accD == 0x000000de);

  dev.close();
}

/**********************************************************************************************************************/

// BOOST_AUTO_TEST_CASE(testAreaHandshake1) {
//   setDMapFilePath("subdeviceTestAreaHandshake.dmap");

//   Device dev;
//   dev.open("SUBDEV4");
//   Device target;
//   target.open("TARGET1");

//   auto acc1 = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER1");
//   auto acc2 = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER2");
//   auto acc3 = dev.getOneDRegisterAccessor<int>("APP.0.MY_AREA1", 6, 0);
//   auto accArea = target.getOneDRegisterAccessor<int32_t>("APP.0.THE_AREA", 10, 0, {AccessMode::raw});
//   auto accS = target.getScalarRegisterAccessor<int32_t>("APP.1.STATUS");
//   std::atomic<bool> done;
//   std::thread t;

//   BOOST_CHECK_THROW(acc1.read(), ChimeraTK::logic_error);
//   std::vector<int> vec = {1, 2, 3, 4, 5, 6};

//   accS = 0;
//   accS.write();
//   done = false;
//   t = std::thread([&] {
//     acc1 = 1897;
//     acc2 = 1897;
//     acc3 = vec;
//     acc1.write();
//     acc2.write();
//     acc3.write();
//     done = true;
//   });
//   BOOST_CHECK(done == false);
//   int countStatusResets = 0;
//   // the dummyForAreaHandshake backend which we use for this test does not set back the status register. we do it
//   // manually from the test, and count how often we need do so Like this we can check that the accessor waits on
//   // status==0 _each_ time before writing, in particular each array entry counts.
//   while(true) {
//     // wait for status=busy
//     do {
//       accS.read();
//       usleep(20000);
//     } while(accS == 0 && !done);
//     if(done) break;
//     countStatusResets++;
//     accS = 0;
//     accS.write();
//   }
//   BOOST_TEST(countStatusResets == 8);
//   t.join();
//   accArea.read();
//   BOOST_CHECK(accArea[0] == 1897);
//   BOOST_CHECK(accArea[1] == 1897 * 4);
//   BOOST_CHECK(accArea[2] == 65536 * vec[0]);
//   BOOST_CHECK(accArea[3] == 65536 * vec[1]);
//   dev.close();
// }

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test2regsScalar) {
  setDMapFilePath("subdeviceTest.dmap");

  Device dev;
  dev.open("SUBDEV_REG_WINDOW_2REG_MODE");
  Device target;
  target.open("TARGET1");

  auto acc2 = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER2");
  auto accA = target.getScalarRegisterAccessor<int32_t>("APP.1.ADDRESS");
  auto accD = target.getScalarRegisterAccessor<int32_t>("APP.1.DATA");

  BOOST_CHECK_THROW(acc2.read(), ChimeraTK::logic_error);
  accA = 42;
  accA.write();

  auto start = std::chrono::steady_clock::now();
  acc2 = 666;
  acc2.write();
  auto stop = std::chrono::steady_clock::now();
  std::chrono::duration<double> diff = stop - start;
  BOOST_CHECK(diff.count() >= 1.0); // sleep time is set to 1 second

  accA.read();
  BOOST_CHECK(accA == 1);
  accD.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(accD), 666 * 4);

  dev.close();
}

/**********************************************************************************************************************/

// BOOST_AUTO_TEST_CASE(testIsFunctional) {
//   setDMapFilePath("subdeviceTest.dmap");

//   Device dev;
//   dev.open("SUBDEV1");
//   Device target;
//   target.open("TARGET1");
//   BOOST_CHECK(dev.isFunctional());
//   dev.setException("Test Exception");
//   // Device should not be functional anymore
//   BOOST_CHECK(!dev.isFunctional());
//   dev.open();
//   BOOST_CHECK(dev.isFunctional());
//   dev.close();
//   BOOST_CHECK(!dev.isFunctional());
// }

/**********************************************************************************************************************/

// BOOST_AUTO_TEST_CASE(TestInvolvedBackendIDs) {
//   setDMapFilePath("subdeviceTest.dmap");

//   ChimeraTK::Device device("SUBDEV1");
//   ChimeraTK::Device target1("TARGET1");

//   auto deviceIDs = device.getInvolvedBackendIDs();
//   BOOST_TEST(deviceIDs.size() == 2);
//   BOOST_TEST(deviceIDs.contains(target1.getBackend()->getBackendID()));
//   BOOST_TEST(deviceIDs.contains(device.getBackend()->getBackendID()));
// }

/**********************************************************************************************************************/

/// Test that the mutual exclusion is working on accessors from two different backend instances which both use the same
/// busy register.
BOOST_AUTO_TEST_CASE(TestMutex) {
  setDMapFilePath("subdeviceTest.dmap");

  ChimeraTK::Device device3("SUBDEV_REG_WINDOW");
  ChimeraTK::Device device1(
      "(subdevice?type=regWindow&device=TARGET1&address=APP.REG_WIN.ADDRESS&writeData=APP.REG_WIN.WRITE_DATA&busy=APP."
      "REG_WIN.BUSY&readRequest=APP.REG_WIN.READ_REQUEST&readData=APP.REG_WIN.READOUT_SINGLE&chipSelectRegister=APP."
      "REG_WIN.CHIP_SELECT&chipIndex=1&map=Subdevice.map)");

  device1.open();
  device3.open();

  auto acc1 = device1.getScalarRegisterAccessor<int32_t>("APP/0/MY_REGISTER1");
  auto acc3 = device3.getScalarRegisterAccessor<int32_t>("APP/0/MY_REGISTER1");

  ChimeraTK::Device targetDevice("TARGET1");
  auto dummyTarget = boost::dynamic_pointer_cast<DummyBackend>(targetDevice.getBackend());

  DummyRegisterAccessor<uint32_t> accChipSelect{dummyTarget.get(), "APP.REG_WIN", "CHIP_SELECT"};
  DummyRegisterAccessor<uint32_t> accWriteData{dummyTarget.get(), "APP.REG_WIN", "WRITE_DATA"};

  boost::barrier write1Reached(2);
  boost::barrier continueWrite1(2);

  std::mutex m;
  std::condition_variable cv;

  bool thread3Started{false};

  std::atomic<size_t> chipSelectCounter{0};
  std::atomic<size_t> writeCounter{0};

  accChipSelect.setWriteCallback([&] { ++chipSelectCounter; });
  accWriteData.setWriteCallback([&] {
    auto previousWriteCount = writeCounter.load();
    if(writeCounter.compare_exchange_weak(previousWriteCount, previousWriteCount + 1) && previousWriteCount == 0) {
      write1Reached.wait();
      continueWrite1.wait();
    }
  });

  std::thread write1([&] { acc1.write(); });
  write1Reached.wait();
  BOOST_REQUIRE(chipSelectCounter == 1);

  std::thread write3([&] {
    {
      std::lock_guard lk(m);
      thread3Started = true;
    }
    cv.notify_one();
    acc3.write();
  });

  {
    std::unique_lock lk(m);
    cv.wait(lk, [&] { return thread3Started; });
  }

  usleep(100000); // Wait a bit for acc3 write to work. It should be blocked

  // The actual test: The chip select register has not been written yet because
  // the register is still blocked.
  BOOST_TEST(chipSelectCounter == 1);

  continueWrite1.wait();

  CHECK_TIMEOUT({}, (chipSelectCounter == 2), 10000);

  write1.join();
  write3.join();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
