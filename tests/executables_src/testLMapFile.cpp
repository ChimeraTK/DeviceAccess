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

void LMapFileTest::testFileNotFound() {
}

void LMapFileTest::testErrorInDmapFile() {
}

void LMapFileTest::testParseFile() {
  LogicalNameMap::RegisterInfo info;

  LogicalNameMap lmap("valid.xlmap");

  info = lmap.getRegisterInfo("SingleWord");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::REGISTER );
  BOOST_CHECK( info.deviceName == "DUMMYD1");
  BOOST_CHECK( info.registerName == "MODULE0.WORD_USER1");

  info = lmap.getRegisterInfo("PartOfArea");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::RANGE );
  BOOST_CHECK( info.deviceName == "PCIE2");
  BOOST_CHECK( info.registerName == "ADC.AREA_DMA_VIA_DMA");
  BOOST_CHECK( info.firstIndex == 10);
  BOOST_CHECK( info.length == 20);

  info = lmap.getRegisterInfo("FullArea");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::REGISTER );
  BOOST_CHECK( info.deviceName == "PCIE2");
  BOOST_CHECK( info.registerName == "ADC.AREA_DMA_VIA_DMA");

  info = lmap.getRegisterInfo("Channel3");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::CHANNEL );
  BOOST_CHECK( info.deviceName == "PCIE3");
  BOOST_CHECK( info.registerName == "TEST.DMA");
  BOOST_CHECK( info.channel == 3);

  info = lmap.getRegisterInfo("Channel4");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::CHANNEL );
  BOOST_CHECK( info.deviceName == "PCIE3");
  BOOST_CHECK( info.registerName == "TEST.DMA");
  BOOST_CHECK( info.channel == 4);

  info = lmap.getRegisterInfo("Constant");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::INT_CONSTANT );
  BOOST_CHECK( info.value == 42);
}
