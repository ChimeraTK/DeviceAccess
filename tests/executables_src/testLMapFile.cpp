/*
 * testLMapFile.cpp
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "LogicalNameMap.h"
#include "DeviceException.h"

using namespace boost::unit_test_framework;
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

      test_case* testFileNotFound = BOOST_CLASS_TEST_CASE(&LMapFileTest::testFileNotFound, lMapFileTest);
      test_case* testErrorInDmapFile = BOOST_CLASS_TEST_CASE(&LMapFileTest::testErrorInDmapFile, lMapFileTest);
      test_case* testParseFile = BOOST_CLASS_TEST_CASE(&LMapFileTest::testParseFile, lMapFileTest);

      add(testFileNotFound);
      add(testErrorInDmapFile);
      add(testParseFile);
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "LogicalNameMap class test suite";
  framework::master_test_suite().add(new LMapFileTestSuite());

  return NULL;
}

void testErrorInDmapFileSingle(std::string fileName) {
  BOOST_CHECK_THROW( LogicalNameMap lmap(fileName), DeviceException );
  try {
    LogicalNameMap lmap(fileName);
  }
  catch(DeviceException &ex) {
    BOOST_CHECK(ex.getID() == DeviceException::CANNOT_OPEN_MAP_FILE);
  }
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
  LogicalNameMap::RegisterInfo info;

  LogicalNameMap lmap("valid.xlmap");

  info = lmap.getRegisterInfo("SingleWord");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::REGISTER );
  BOOST_CHECK( info.deviceName == "PCIE2");
  BOOST_CHECK( info.registerName == "BOARD.WORD_USER");
  BOOST_CHECK( info.hasDeviceName() == true );
  BOOST_CHECK( info.hasRegisterName() == true );
  BOOST_CHECK( info.hasFirstIndex() == false );
  BOOST_CHECK( info.hasLength() == false );
  BOOST_CHECK( info.hasChannel() == false );
  BOOST_CHECK( info.hasValue() == false );

  info = lmap.getRegisterInfo("PartOfArea");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::RANGE );
  BOOST_CHECK( info.deviceName == "PCIE2");
  BOOST_CHECK( info.registerName == "ADC.AREA_DMAABLE");
  BOOST_CHECK( info.firstIndex == 10);
  BOOST_CHECK( info.length == 20);
  BOOST_CHECK( info.hasDeviceName() == true );
  BOOST_CHECK( info.hasRegisterName() == true );
  BOOST_CHECK( info.hasFirstIndex() == true );
  BOOST_CHECK( info.hasLength() == true );
  BOOST_CHECK( info.hasChannel() == false );
  BOOST_CHECK( info.hasValue() == false );

  info = lmap.getRegisterInfo("FullArea");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::REGISTER );
  BOOST_CHECK( info.deviceName == "PCIE2");
  BOOST_CHECK( info.registerName == "ADC.AREA_DMAABLE");
  BOOST_CHECK( info.hasDeviceName() == true );
  BOOST_CHECK( info.hasRegisterName() == true );
  BOOST_CHECK( info.hasFirstIndex() == false );
  BOOST_CHECK( info.hasLength() == false );
  BOOST_CHECK( info.hasChannel() == false );
  BOOST_CHECK( info.hasValue() == false );

  info = lmap.getRegisterInfo("Channel3");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::CHANNEL );
  BOOST_CHECK( info.deviceName == "PCIE3");
  BOOST_CHECK( info.registerName == "TEST.NODMA");
  BOOST_CHECK( info.channel == 3);
  BOOST_CHECK( info.hasDeviceName() == true );
  BOOST_CHECK( info.hasRegisterName() == true );
  BOOST_CHECK( info.hasFirstIndex() == false );
  BOOST_CHECK( info.hasLength() == false );
  BOOST_CHECK( info.hasChannel() == true );
  BOOST_CHECK( info.hasValue() == false );

  info = lmap.getRegisterInfo("Channel4");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::CHANNEL );
  BOOST_CHECK( info.deviceName == "PCIE3");
  BOOST_CHECK( info.registerName == "TEST.NODMA");
  BOOST_CHECK( info.channel == 4);
  BOOST_CHECK( info.hasDeviceName() == true );
  BOOST_CHECK( info.hasRegisterName() == true );
  BOOST_CHECK( info.hasFirstIndex() == false );
  BOOST_CHECK( info.hasLength() == false );
  BOOST_CHECK( info.hasChannel() == true );
  BOOST_CHECK( info.hasValue() == false );

  info = lmap.getRegisterInfo("Constant");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::INT_CONSTANT );
  BOOST_CHECK( info.value == 42);
  BOOST_CHECK( info.hasDeviceName() == false );
  BOOST_CHECK( info.hasRegisterName() == false );
  BOOST_CHECK( info.hasFirstIndex() == false );
  BOOST_CHECK( info.hasLength() == false );
  BOOST_CHECK( info.hasChannel() == false );
  BOOST_CHECK( info.hasValue() == true );

  std::unordered_set<std::string> targetDevices = lmap.getTargetDevices();
  BOOST_CHECK(targetDevices.size() == 2);
  BOOST_CHECK(targetDevices.count("PCIE2") == 1);
  BOOST_CHECK(targetDevices.count("PCIE3") == 1);

  BOOST_CHECK_THROW( lmap.getRegisterInfo("NotExistingRegister"), DeviceException );
  try {
    lmap.getRegisterInfo("NotExistingRegister");
  }
  catch(DeviceException &ex) {
    BOOST_CHECK( ex.getID() == DeviceException::REGISTER_DOES_NOT_EXIST );
  }

}
