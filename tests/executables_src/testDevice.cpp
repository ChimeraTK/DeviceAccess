// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DeviceTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DeviceBackend.h"
#include "DummyRegisterAccessor.h"
#include "Exception.h"

#include <cstring>

namespace ChimeraTK {
  using namespace ChimeraTK;
}

class TestableDevice : public ChimeraTK::Device {
 public:
  boost::shared_ptr<ChimeraTK::DeviceBackend> getBackend() { return _deviceBackendPointer; }
};

BOOST_AUTO_TEST_SUITE(DeviceTestSuite)

BOOST_AUTO_TEST_CASE(testConvenienceReadWrite) {
  ChimeraTK::setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;
  device.open("DUMMYD2");
  boost::shared_ptr<ChimeraTK::DummyBackend> backend = boost::dynamic_pointer_cast<ChimeraTK::DummyBackend>(
      ChimeraTK::BackendFactory::getInstance().createBackend("DUMMYD2"));

  ChimeraTK::DummyRegisterAccessor<int32_t> wordStatus(backend.get(), "APP0", "WORD_STATUS");
  ChimeraTK::DummyRegisterAccessor<int32_t> module0(backend.get(), "APP0", "MODULE0");

  int32_t data;
  std::vector<int32_t> dataVector;

  wordStatus = 0x444d4d59;
  data = device.read<int32_t>("APP0.WORD_STATUS");
  BOOST_CHECK(data == 0x444d4d59);

  wordStatus = -42;
  data = device.read<int32_t>("APP0.WORD_STATUS");
  BOOST_CHECK(data == -42);

  module0[0] = 120;
  module0[1] = 0xDEADBEEF;

  data = device.read<int32_t>("APP0/MODULE0");
  BOOST_CHECK(data == 120);

  dataVector = device.read<int32_t>("APP0/MODULE0", 2, 0);
  BOOST_CHECK(dataVector.size() == 2);
  BOOST_CHECK(dataVector[0] == 120);
  BOOST_CHECK(dataVector[1] == static_cast<signed>(0xDEADBEEF));

  module0[0] = 66;
  module0[1] = -33333;

  dataVector = device.read<int32_t>("APP0/MODULE0", 1, 0);
  BOOST_CHECK(dataVector.size() == 1);
  BOOST_CHECK(dataVector[0] == 66);

  dataVector = device.read<int32_t>("APP0/MODULE0", 1, 1);
  BOOST_CHECK(dataVector.size() == 1);
  BOOST_CHECK(dataVector[0] == -33333);

  BOOST_CHECK_THROW([[maybe_unused]] auto x = device.read<int32_t>("APP0/DOESNT_EXIST"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      [[maybe_unused]] auto x = device.read<int32_t>("DOESNT_EXIST/AT_ALL", 1, 0), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testDeviceCreation) {
  std::string initialDmapFilePath = ChimeraTK::BackendFactory::getInstance().getDMapFilePath();
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("dMapDir/testRelativePaths.dmap");

  ChimeraTK::Device device1;
  BOOST_CHECK(device1.isOpened() == false);
  device1.open("DUMMYD0");
  BOOST_CHECK(device1.isOpened() == true);
  BOOST_CHECK_NO_THROW(device1.open("DUMMYD0"));
  { // scope to have a device which goes out of scope
    ChimeraTK::Device device1a;
    // open the same backend than device1
    device1a.open("DUMMYD0");
    BOOST_CHECK(device1a.isOpened() == true);
  }
  // check that device1 has not been closed by device 1a going out of scope
  BOOST_CHECK(device1.isOpened() == true);

  ChimeraTK::Device device1b;
  // open the same backend than device1
  device1b.open("DUMMYD0");
  // open another backend with the same device //ugly, might be deprecated soon
  device1b.open("DUMMYD0");
  // check that device1 has not been closed by device 1b being reassigned
  BOOST_CHECK(device1.isOpened() == true);

  ChimeraTK::Device device2;
  BOOST_CHECK(device2.isOpened() == false);
  device2.open("DUMMYD1");
  BOOST_CHECK(device2.isOpened() == true);
  BOOST_CHECK_NO_THROW(device2.open("DUMMYD1"));
  BOOST_CHECK(device2.isOpened() == true);

  ChimeraTK::Device device3;
  BOOST_CHECK(device3.isOpened() == false);
  BOOST_CHECK_NO_THROW(device3.open("DUMMYD0"));
  BOOST_CHECK(device3.isOpened() == true);
  ChimeraTK::Device device4;
  BOOST_CHECK(device4.isOpened() == false);
  BOOST_CHECK_NO_THROW(device4.open("DUMMYD1"));
  BOOST_CHECK(device4.isOpened() == true);

  // check if opening without alias name fails
  TestableDevice device5;
  BOOST_CHECK(device5.isOpened() == false);
  BOOST_CHECK_THROW(device5.open(), ChimeraTK::logic_error);
  BOOST_CHECK(device5.isOpened() == false);
  BOOST_CHECK_THROW(device5.open(), ChimeraTK::logic_error);
  BOOST_CHECK(device5.isOpened() == false);

  // check if opening device with different backend keeps old backend open.
  BOOST_CHECK_NO_THROW(device5.open("DUMMYD0"));
  BOOST_CHECK(device5.isOpened() == true);
  auto backend5 = device5.getBackend();
  BOOST_CHECK_NO_THROW(device5.open("DUMMYD1"));
  BOOST_CHECK(backend5->isOpen()); // backend5 is still the current backend of device5
  BOOST_CHECK(device5.isOpened() == true);

  // check closing and opening again
  backend5 = device5.getBackend();
  BOOST_CHECK(backend5->isOpen());
  BOOST_CHECK(device5.isOpened() == true);
  device5.close();
  BOOST_CHECK(device5.isOpened() == false);
  BOOST_CHECK(!backend5->isOpen());
  device5.open();
  BOOST_CHECK(device5.isOpened() == true);
  BOOST_CHECK(backend5->isOpen());

  // Now that we are done with the tests, move the factory to the state it was
  // in before we started
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath(initialDmapFilePath);
}

#if 0
BOOST_AUTO_TEST_CASE(testDeviceInfo) {
  ChimeraTK::Device device;
  device.open("DUMMYD3");
  std::string deviceInfo = device.readDeviceInfo();
  std::cout << deviceInfo << std::endl;
  BOOST_CHECK(deviceInfo.substr(0, 31) == "DummyBackend with mapping file ");
}
#endif

BOOST_AUTO_TEST_CASE(testIsFunctional) {
  ChimeraTK::Device d;
  // a disconnected device is not functional
  BOOST_CHECK(d.isFunctional() == false);

  d.open("DUMMYD1");
  BOOST_CHECK(d.isFunctional() == true);

  d.close();
  BOOST_CHECK(d.isFunctional() == false);
}

BOOST_AUTO_TEST_SUITE_END()
