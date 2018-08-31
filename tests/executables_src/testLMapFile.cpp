///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include "LogicalNameMapParser.h"
#include "LNMBackendRegisterInfo.h"
#include "Exception.h"

using namespace boost::unit_test_framework;
namespace mtca4u{
  using namespace ChimeraTK;
}
using namespace mtca4u;

class LMapFileTest {
  public:
    void testFileNotFound();
    void testErrorInDmapFile();
    void testParseFile();
};

class LMapFileTestSuite : public test_suite {
  public:
    LMapFileTestSuite() : test_suite("LogicalNameMap class test suite") {
      boost::shared_ptr<LMapFileTest> lMapFileTest(new LMapFileTest);

      add( BOOST_CLASS_TEST_CASE(&LMapFileTest::testFileNotFound, lMapFileTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapFileTest::testErrorInDmapFile, lMapFileTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapFileTest::testParseFile, lMapFileTest) );
    }
};

bool init_unit_test(){
  framework::master_test_suite().p_name.value = "LogicalNameMap class test suite";
  framework::master_test_suite().add(new LMapFileTestSuite());

  return true;
}

void testErrorInDmapFileSingle(std::string fileName) {
  BOOST_CHECK_THROW( LogicalNameMapParser lmap(fileName), ChimeraTK::logic_error );
}

void LMapFileTest::testFileNotFound() {
  testErrorInDmapFileSingle("notExisting.xlmap");
}

void LMapFileTest::testErrorInDmapFile() {
  testErrorInDmapFileSingle("invalid1.xlmap");
  testErrorInDmapFileSingle("invalid2.xlmap");
  testErrorInDmapFileSingle("invalid3.xlmap");
  testErrorInDmapFileSingle("invalid4.xlmap");
  testErrorInDmapFileSingle("invalid5.xlmap");
  testErrorInDmapFileSingle("invalid6.xlmap");
  testErrorInDmapFileSingle("invalid7.xlmap");
  testErrorInDmapFileSingle("invalid8.xlmap");
}

void LMapFileTest::testParseFile() {
  boost::shared_ptr<LNMBackendRegisterInfo> info;

  LogicalNameMapParser lmap("valid.xlmap");
  RegisterCatalogue catalogue = lmap.getCatalogue();

  info =  boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("SingleWord"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER );
  BOOST_CHECK( info->deviceName == "PCIE2");
  BOOST_CHECK( info->registerName == "BOARD.WORD_USER");

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("PartOfArea"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER );
  BOOST_CHECK( info->deviceName == "PCIE2");
  BOOST_CHECK( info->registerName == "ADC.AREA_DMAABLE");
  BOOST_CHECK( info->firstIndex == 10);
  BOOST_CHECK( info->length == 20);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("FullArea"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER );
  BOOST_CHECK( info->deviceName == "PCIE2");
  BOOST_CHECK( info->registerName == "ADC.AREA_DMAABLE");

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Channel3"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL );
  BOOST_CHECK( info->deviceName == "PCIE3");
  BOOST_CHECK( info->registerName == "TEST.NODMA");
  BOOST_CHECK( info->channel == 3);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Channel4"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL );
  BOOST_CHECK( info->deviceName == "PCIE3");
  BOOST_CHECK( info->registerName == "TEST.NODMA");
  BOOST_CHECK( info->channel == 4);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Constant"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::INT_CONSTANT );
  BOOST_CHECK( info->value_int[0] == 42);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("/MyModule/SomeSubmodule/Variable"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::INT_VARIABLE );
  BOOST_CHECK( info->value_int[0] == 2);
  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("MyModule/ConfigurableChannel"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL );
  BOOST_CHECK( info->deviceName == "PCIE3");
  BOOST_CHECK( info->registerName == "TEST.NODMA");
  BOOST_CHECK( info->channel == 42);

  std::unordered_set<std::string> targetDevices = lmap.getTargetDevices();
  BOOST_CHECK(targetDevices.size() == 2);
  BOOST_CHECK(targetDevices.count("PCIE2") == 1);
  BOOST_CHECK(targetDevices.count("PCIE3") == 1);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("ArrayConstant"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::INT_CONSTANT );
  BOOST_CHECK( info->length == 5);
  BOOST_CHECK( info->value_int.size() == 5);
  BOOST_CHECK( info->value_int[0] == 1111);
  BOOST_CHECK( info->value_int[1] == 2222);
  BOOST_CHECK( info->value_int[2] == 3333);
  BOOST_CHECK( info->value_int[3] == 4444);
  BOOST_CHECK( info->value_int[4] == 5555);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit0ofVar"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::BIT );
  BOOST_CHECK( info->deviceName == "this");
  BOOST_CHECK( info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK( info->bit == 0);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit1ofVar"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::BIT );
  BOOST_CHECK( info->deviceName == "this");
  BOOST_CHECK( info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK( info->bit == 1);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit2ofVar"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::BIT );
  BOOST_CHECK( info->deviceName == "this");
  BOOST_CHECK( info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK( info->bit == 2);

  info = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(catalogue.getRegister("Bit3ofVar"));
  BOOST_CHECK( info->targetType == LNMBackendRegisterInfo::TargetType::BIT );
  BOOST_CHECK( info->deviceName == "this");
  BOOST_CHECK( info->registerName == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK( info->bit == 3);
}
