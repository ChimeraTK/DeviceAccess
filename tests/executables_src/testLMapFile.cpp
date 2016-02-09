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

  info = lmap.getRegisterInfo("Test1");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::REGISTER );
  BOOST_CHECK( info.deviceName == "BOARD0");
  BOOST_CHECK( info.registerName == "REGISTER1");

  info = lmap.getRegisterInfo("Test2");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::CHANNEL );
  BOOST_CHECK( info.deviceName == "BOARD0");
  BOOST_CHECK( info.registerName == "AREA2");
  BOOST_CHECK( info.channel == 42);

  info = lmap.getRegisterInfo("Test3");
  BOOST_CHECK( info.targetType == LogicalNameMap::TargetType::INT_CONSTANT );
  BOOST_CHECK( info.value == 12);
}
