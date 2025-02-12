// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapTagModifierPluginTest

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"

#include <iostream>

using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE(testParameters) {
  std::cout << "testParameters" << std::endl;

  ChimeraTK::Device device;
  BOOST_CHECK_THROW(device.open("(logicalNameMap?map=tagModifierPluginNoParameters.xlmap)"), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testAddRemove) {
  std::cout << "testAddRemove" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("test");
  auto tags = info.getTags();

  std::set<std::string> reference = {"flower", "mountain", "no-recover", "rumpelstiltzchen", "status-output"};

  BOOST_CHECK_EQUAL_COLLECTIONS(tags.begin(), tags.end(), reference.begin(), reference.end());
}

BOOST_AUTO_TEST_CASE(testSet) {
  std::cout << "testSet" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("set");
  auto tags = info.getTags();

  std::set<std::string> reference = {"no-recover", "status-output", "random", "triggered", "main", "test", "none"};
  BOOST_CHECK_EQUAL_COLLECTIONS(tags.begin(), tags.end(), reference.begin(), reference.end());
}

BOOST_AUTO_TEST_CASE(testAdd) {
  std::cout << "testAdd" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("add");
  auto tags = info.getTags();

  std::set<std::string> reference = {
      "no-recover", "status-output", "random", "triggered", "main", "test", "none", "self-service", "shower"};
  BOOST_CHECK_EQUAL_COLLECTIONS(tags.begin(), tags.end(), reference.begin(), reference.end());
}

BOOST_AUTO_TEST_CASE(testRemove) {
  std::cout << "testRemove" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("remove");
  auto tags = info.getTags();

  std::set<std::string> reference = {"no-recover", "status-output", "random", "triggered", "test"};
  BOOST_CHECK_EQUAL_COLLECTIONS(tags.begin(), tags.end(), reference.begin(), reference.end());
}