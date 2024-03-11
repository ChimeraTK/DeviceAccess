// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapTypeHintModifierPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapForceReadOnlyPluginTestSuite)

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test) {
  std::cout << "test" << std::endl;
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=typeHintModifierPlugin.xlmap)");

  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("test");
  auto descriptor = info.getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 11);

  // Basically check if integer aliases to int32
  info = cat.getRegister("test2");
  descriptor = info.getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 11);

  info = cat.getRegister("test3");
  descriptor = info.getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(not descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 20);

  info = cat.getRegister("test4");
  descriptor = info.getDataDescriptor();
  BOOST_CHECK(not descriptor.isIntegral());
  BOOST_CHECK(descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nFractionalDigits(), 325);
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 328);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWithMathPlugin) {
  std::cout << "testWithMathPlugin" << std::endl;
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=typeHintModifierPlugin.xlmap)");

  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("testWithMathPlugin");
  auto descriptor = info.getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(!descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 5);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
