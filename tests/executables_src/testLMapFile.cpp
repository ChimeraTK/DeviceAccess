#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMapParser.h"

using namespace ChimeraTK;

/********************************************************************************************************************/

void testErrorInDmapFileSingle(std::string fileName) {
  BOOST_CHECK_THROW(LogicalNameMapParser lmap(fileName, {}), ChimeraTK::logic_error);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFileNotFound) {
  testErrorInDmapFileSingle("notExisting.xlmap");
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testErrorInDmapFile) {
  testErrorInDmapFileSingle("invalid1.xlmap");
  testErrorInDmapFileSingle("invalid2.xlmap");
  testErrorInDmapFileSingle("invalid3.xlmap");
  testErrorInDmapFileSingle("invalid4.xlmap");
  testErrorInDmapFileSingle("invalid5.xlmap");
  testErrorInDmapFileSingle("invalid6.xlmap");
  testErrorInDmapFileSingle("invalid7.xlmap");
  testErrorInDmapFileSingle("invalid8.xlmap");
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParseFile) {
  boost::shared_ptr<LNMBackendRegisterInfo> info;

  LogicalNameMapParser lmap("valid.xlmap", {});
  RegisterCatalogue catalogue = lmap.getCatalogue();

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("SingleWord"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info->deviceName == "PCIE2");
  BOOST_CHECK(info->registerName == "BOARD.WORD_USER");

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("PartOfArea"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info->deviceName == "PCIE2");
  BOOST_CHECK(info->registerName == "ADC.AREA_DMAABLE");
  BOOST_CHECK(info->firstIndex == 10);
  BOOST_CHECK(info->length == 20);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("FullArea"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
  BOOST_CHECK(info->deviceName == "PCIE2");
  BOOST_CHECK(info->registerName == "ADC.AREA_DMAABLE");

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Channel3"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);
  BOOST_CHECK(info->deviceName == "PCIE3");
  BOOST_CHECK(info->registerName == "TEST.NODMA");
  BOOST_CHECK(info->channel == 3);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Channel4"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);
  BOOST_CHECK(info->deviceName == "PCIE3");
  BOOST_CHECK(info->registerName == "TEST.NODMA");
  BOOST_CHECK(info->channel == 4);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Constant"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT);
  BOOST_CHECK(info->valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[0] == 42);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("/MyModule/SomeSubmodule/Variable"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::VARIABLE);
  BOOST_CHECK(info->valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[0] == 2);
  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("MyModule/ConfigurableChannel"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);
  BOOST_CHECK(info->deviceName == "PCIE3");
  BOOST_CHECK(info->registerName == "TEST.NODMA");
  BOOST_CHECK(info->channel == 42);

  std::unordered_set<std::string> targetDevices = lmap.getTargetDevices();
  BOOST_CHECK(targetDevices.size() == 2);
  BOOST_CHECK(targetDevices.count("PCIE2") == 1);
  BOOST_CHECK(targetDevices.count("PCIE3") == 1);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("ArrayConstant"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT);
  BOOST_CHECK(info->valueType == ChimeraTK::DataType::int32);
  BOOST_CHECK(info->length == 5);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table).size() == 5);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[0] == 1111);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[1] == 2222);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[2] == 3333);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[3] == 4444);
  BOOST_CHECK(boost::fusion::at_key<int32_t>(info->valueTable.table)[4] == 5555);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit0ofVar"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info->deviceName == "this");
  BOOST_CHECK(info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info->bit == 0);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit1ofVar"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info->deviceName == "this");
  BOOST_CHECK(info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info->bit == 1);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit2ofVar"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info->deviceName == "this");
  BOOST_CHECK(info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info->bit == 2);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit3ofVar"));
  BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::BIT);
  BOOST_CHECK(info->deviceName == "this");
  BOOST_CHECK(info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info->bit == 3);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParameters) {
  boost::shared_ptr<LNMBackendRegisterInfo> info;

  {
    std::map<std::string, std::string> params;
    params["ParamA"] = "ValueA";
    params["ParamB"] = "ValueB";

    LogicalNameMapParser lmap("withParams.xlmap", params);
    RegisterCatalogue catalogue = lmap.getCatalogue();

    info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("SingleWordWithParams"));
    BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
    BOOST_CHECK(info->deviceName == "ValueA");
    BOOST_CHECK(info->registerName == "ValueB");
  }

  {
    std::map<std::string, std::string> params;
    params["ParamA"] = "OtherValues";
    params["ParamB"] = "ThisTime";

    LogicalNameMapParser lmap("withParams.xlmap", params);
    RegisterCatalogue catalogue = lmap.getCatalogue();

    info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("SingleWordWithParams"));
    BOOST_CHECK(info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER);
    BOOST_CHECK(info->deviceName == "OtherValues");
    BOOST_CHECK(info->registerName == "ThisTime");
  }
}
