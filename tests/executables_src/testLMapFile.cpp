// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMapParser.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapFileTestSuite)

/********************************************************************************************************************/

void testErrorInDmapFileSingle(std::string fileName) {
  std::map<std::string, LNMVariable> variables;
  LogicalNameMapParser lmap({}, variables);
  BOOST_CHECK_THROW(lmap.parseFile(fileName), ChimeraTK::logic_error);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFileNotFound) {
  std::cout << "******************************************************" << std::endl;
  std::cout << "*** Warnings ahead. Testing for not existing file. ***" << std::endl;
  testErrorInDmapFileSingle("notExisting.xlmap");
  std::cout << "*** End of not existing file test. *******************" << std::endl;
  std::cout << "******************************************************" << std::endl;
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testErrorInDmapFile) {
  std::cout << "********************************************************" << std::endl;
  std::cout << "*** Warnings ahead. Testing for invalid xlmap files. ***" << std::endl;
  testErrorInDmapFileSingle("invalid1.xlmap");
  testErrorInDmapFileSingle("invalid2.xlmap");
  testErrorInDmapFileSingle("invalid3.xlmap");
  testErrorInDmapFileSingle("invalid4.xlmap");
  testErrorInDmapFileSingle("invalid5.xlmap");
  testErrorInDmapFileSingle("invalid6.xlmap");
  testErrorInDmapFileSingle("invalid7.xlmap");
  testErrorInDmapFileSingle("invalid8.xlmap");
  testErrorInDmapFileSingle("invalidStartIndex1.xlmap");
  testErrorInDmapFileSingle("invalidStartIndex2.xlmap");
  testErrorInDmapFileSingle("invalidDuplicateName.xlmap");
  std::cout << "*** End of invalid xlmap file test. ********************" << std::endl;
  std::cout << "********************************************************" << std::endl;
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParseFile) {
  std::map<std::string, LNMVariable> variables;
  LogicalNameMapParser lmap({}, variables);
  auto catalogue = lmap.parseFile("valid.xlmap");

  auto info = catalogue.getBackendRegister("SingleWord");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info.deviceName == "PCIE2");
  BOOST_CHECK(info.registerName == "BOARD.WORD_USER");

  info = catalogue.getBackendRegister("PartOfArea");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info.deviceName == "PCIE2");
  BOOST_CHECK(info.registerName == "ADC.AREA_DMAABLE");
  BOOST_CHECK(info.firstIndex == 10);
  BOOST_CHECK(info.length == 20);

  info = catalogue.getBackendRegister("FullArea");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info.deviceName == "PCIE2");
  BOOST_CHECK(info.registerName == "ADC.AREA_DMAABLE");

  info = catalogue.getBackendRegister("usingHexStartIndex");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info.registerName == "ADC.AREA_DMAABLE");
  BOOST_CHECK(info.firstIndex == 0x10);

  info = catalogue.getBackendRegister("Channel3");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);
  BOOST_CHECK(info.deviceName == "PCIE3");
  BOOST_CHECK(info.registerName == "TEST.NODMA");
  BOOST_CHECK(info.channel == 3);

  info = catalogue.getBackendRegister("Channel4");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);
  BOOST_CHECK(info.deviceName == "PCIE3");
  BOOST_CHECK(info.registerName == "TEST.NODMA");
  BOOST_CHECK(info.channel == 4);

  info = catalogue.getBackendRegister("Constant");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::CONSTANT);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[0] == 42);

  info = catalogue.getBackendRegister("/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[0] == 2);
  info = catalogue.getBackendRegister("MyModule/ConfigurableChannel");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);
  BOOST_CHECK(info.deviceName == "PCIE3");
  BOOST_CHECK(info.registerName == "TEST.NODMA");
  BOOST_CHECK(info.channel == 42);

  info = catalogue.getBackendRegister("ArrayConstant");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::CONSTANT);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(info.length == 5);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue.size() == 5);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[0] == 1111);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[1] == 2222);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[2] == 3333);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[3] == 4444);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[4] == 5555);

  info = catalogue.getBackendRegister("Bit0ofVar");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info.deviceName == "this");
  BOOST_CHECK(info.registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.bit == 0);

  info = catalogue.getBackendRegister("Bit1ofVar");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info.deviceName == "this");
  BOOST_CHECK(info.registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.bit == 1);

  info = catalogue.getBackendRegister("Bit2ofVar");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info.deviceName == "this");
  BOOST_CHECK(info.registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.bit == 2);

  info = catalogue.getBackendRegister("Bit3ofVar");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info.deviceName == "this");
  BOOST_CHECK(info.registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.bit == 3);

  info = catalogue.getBackendRegister("/var_int8");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int8);
  BOOST_CHECK(boost::fusion::at_key<int8_t>(variables.at(info.name).valueTable.table).latestValue[0] == -128);
  info = catalogue.getBackendRegister("/var_uint8");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::uint8);
  BOOST_CHECK(boost::fusion::at_key<uint8_t>(variables.at(info.name).valueTable.table).latestValue[0] == 255);
  info = catalogue.getBackendRegister("/var_int16");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int16);
  BOOST_CHECK(boost::fusion::at_key<int16_t>(variables.at(info.name).valueTable.table).latestValue[0] == -32768);
  info = catalogue.getBackendRegister("/var_uint16");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::uint16);
  BOOST_CHECK(boost::fusion::at_key<uint16_t>(variables.at(info.name).valueTable.table).latestValue[0] == 65535);
  info = catalogue.getBackendRegister("/var_int32");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(variables.at(info.name).valueTable.table).latestValue[0] == -1);
  info = catalogue.getBackendRegister("/var_uint32");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::uint32);
  BOOST_CHECK(boost::fusion::at_key<uint32_t>(variables.at(info.name).valueTable.table).latestValue[0] == 0xfffffffe);
  info = catalogue.getBackendRegister("/var_int64");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::int64);
  BOOST_CHECK(boost::fusion::at_key<int64_t>(variables.at(info.name).valueTable.table).latestValue[0] == -1);
  info = catalogue.getBackendRegister("/var_uint64");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::uint64);
  BOOST_CHECK(boost::fusion::at_key<uint64_t>(variables.at(info.name).valueTable.table).latestValue[0] == 0);
  info = catalogue.getBackendRegister("/var_string");
  BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info.valueType == ChimeraTK::DataType::string);
  BOOST_CHECK(
      boost::fusion::at_key<std::string>(variables.at(info.name).valueTable.table).latestValue[0] == "stringValue");
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParameters) {
  {
    std::map<std::string, std::string> params;
    params["ParamA"] = "ValueA";
    params["ParamB"] = "ValueB";
    std::map<std::string, LNMVariable> variables;

    LogicalNameMapParser lmap(params, variables);
    auto catalogue = lmap.parseFile("withParams.xlmap");

    auto info = catalogue.getBackendRegister("SingleWordWithParams");
    BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
    BOOST_CHECK(info.deviceName == "ValueA");
    BOOST_CHECK(info.registerName == "ValueB");
  }

  {
    std::map<std::string, std::string> params;
    params["ParamA"] = "OtherValues";
    params["ParamB"] = "ThisTime";
    std::map<std::string, LNMVariable> variables;

    LogicalNameMapParser lmap(params, variables);
    auto catalogue = lmap.parseFile("withParams.xlmap");

    auto info = catalogue.getBackendRegister("SingleWordWithParams");
    BOOST_CHECK(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
    BOOST_CHECK(info.deviceName == "OtherValues");
    BOOST_CHECK(info.registerName == "ThisTime");
  }
}

BOOST_AUTO_TEST_SUITE_END()
