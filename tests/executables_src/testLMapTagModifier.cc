// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapTagModifierPluginTest

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "SystemTags.h"

#include <iostream>

using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE(testNoParameters) {
  std::cout << "testNoParameters" << std::endl;

  ChimeraTK::Device device;
  BOOST_CHECK_THROW(device.open("(logicalNameMap?map=tagModifierPluginNoParameters.xlmap)"), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testAddRemove) {
  std::cout << "testAddRemove" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto baseInfo = cat.getRegister("plain");
  BOOST_TEST(baseInfo.getTags().empty());

  auto info = cat.getRegister("addRemove");
  auto tags = info.getTags();

  std::set<std::string> reference = {"flower", "mountain", "no-recover", "rumpelstilzchen", "status-output"};

  BOOST_TEST(tags == reference, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(testSet) {
  std::cout << "testSet" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto baseInfo = cat.getRegister("baseline");
  std::set<std::string> baseReference = {"one", "two", "three"};
  BOOST_TEST(baseInfo.getTags() == baseReference, boost::test_tools::per_element());

  auto info = cat.getRegister("set");
  auto tags = info.getTags();

  std::set<std::string> reference = {"no-recover", "status-output", "main", "test"};
  BOOST_TEST(tags == reference, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(testAdd) {
  std::cout << "testAdd" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto baseInfo = cat.getRegister("set");
  auto baseTagsReference = std::set<std::string>({"no-recover", "status-output", "main", "test"});
  BOOST_TEST(baseInfo.getTags() == baseTagsReference, boost::test_tools::per_element());

  auto info = cat.getRegister("add");
  auto tags = info.getTags();

  std::set<std::string> reference = {
      "no-recover", "status-output", "main", "test", "do-something", "update-request", "interrupted", "other"};
  BOOST_TEST(info.getTags() == reference, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(testRemove) {
  std::cout << "testRemove" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto baseInfo = cat.getRegister("set");
  auto baseTagsReference = std::set<std::string>({"no-recover", "status-output", "main", "test"});
  BOOST_TEST(baseInfo.getTags() == baseTagsReference, boost::test_tools::per_element());

  auto info = cat.getRegister("remove");
  auto tags = info.getTags();

  std::set<std::string> reference = {"main", "test"};
  BOOST_TEST(info.getTags() == reference, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(testConvenienceTags) {
  std::cout << "testConveniencePlugins" << std::endl;

  ChimeraTK::Device device;

  device.open("(logicalNameMap?map=tagModifierPlugin.xlmap)");
  auto cat = device.getRegisterCatalogue();
  auto baseInfo = cat.getRegister("convenienceReverse");
  auto baseTagsReference = std::set<std::string>({ChimeraTK::SystemTags::reverseRecovery});
  BOOST_TEST(baseInfo.getTags() == baseTagsReference, boost::test_tools::per_element());

  baseInfo = cat.getRegister("convenienceStatusOutput");
  baseTagsReference = std::set<std::string>({ChimeraTK::SystemTags::statusOutput});
  BOOST_TEST(baseInfo.getTags() == baseTagsReference, boost::test_tools::per_element());
}
