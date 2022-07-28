// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE PcieErrorHandling
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DeviceBackend.h"
#include "DummyRegisterAccessor.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "PcieBackend.h"

#include <cstring>

namespace ChimeraTK {
  using namespace ChimeraTK;
}

BOOST_AUTO_TEST_CASE(testPcieErrorHandling) {
  std::string line;

  ChimeraTK::setDMapFilePath("pcie_device.dmap");
  ChimeraTK::Device device("PCIE0");
  auto reg = device.getScalarRegisterAccessor<uint32_t>("APP.0.WORD_FIRMWARE");

  BOOST_CHECK(!device.isFunctional());
  device.open();

  BOOST_CHECK(device.isFunctional());
  reg.read();
  BOOST_CHECK(reg != 0);
  BOOST_CHECK(device.isFunctional());

  std::cout << "Please now pull the hotplug handle or shut the board down via the MCH, then press ENTER...";
  std::getline(std::cin, line);

  BOOST_CHECK(!device.isFunctional());
  BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(!device.isFunctional());
  BOOST_CHECK_THROW(device.open(), ChimeraTK::runtime_error);
  BOOST_CHECK(!device.isFunctional());

  std::cout << "Please now push the hotplug handle back in or start the board via the MCH, then press ENTER...";
  std::getline(std::cin, line);

  BOOST_CHECK(!device.isFunctional());
  device.open();
  BOOST_CHECK(device.isFunctional());

  BOOST_CHECK(device.isFunctional());
  reg.read();
  BOOST_CHECK(reg != 0);
  BOOST_CHECK(device.isFunctional());
}
