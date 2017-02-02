#include <boost/test/included/unit_test.hpp>

#include "DMapFileParser.h"
#include "MapException.h"
#include "helperFunctions.h"
#include "parserUtilities.h"
#include "Utilities.h"

using namespace boost::unit_test_framework;

class DMapFileParserTest {
  public:
    void testFileNotFound();
    void testErrorInDmapFile();
    void testNoDataInDmapFile();
    void testParseFile();
};

class DMapFileParserTestSuite : public test_suite {
  public:
    DMapFileParserTestSuite() : test_suite("DMapFileParser class test suite") {
      boost::shared_ptr<DMapFileParserTest> dMapFileParserTest(
          new DMapFileParserTest);

      test_case* testFileNotFound = BOOST_CLASS_TEST_CASE(
          &DMapFileParserTest::testFileNotFound, dMapFileParserTest);
      test_case* testErrorInDmapFile = BOOST_CLASS_TEST_CASE(
          &DMapFileParserTest::testErrorInDmapFile, dMapFileParserTest);
      test_case* testNoDataInDmapFile = BOOST_CLASS_TEST_CASE(
          &DMapFileParserTest::testNoDataInDmapFile, dMapFileParserTest);
      test_case* testParseFile = BOOST_CLASS_TEST_CASE(
          &DMapFileParserTest::testParseFile, dMapFileParserTest);

      add(testFileNotFound);
      add(testErrorInDmapFile);
      add(testParseFile);
      add(testNoDataInDmapFile);
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value =
      "DMapFileParser class test suite";
  framework::master_test_suite().add(new DMapFileParserTestSuite());

  return NULL;
}

void DMapFileParserTest::testFileNotFound() {
  std::string file_path = "../dummypath.dmap";
  mtca4u::DMapFileParser fileParser;

  BOOST_CHECK_THROW(fileParser.parse(file_path), mtca4u::LibMapException);
  try {
    fileParser.parse(file_path);
  }
  catch (mtca4u::LibMapException& dMapFileParserException) {
    BOOST_CHECK(dMapFileParserException.getID() ==
        mtca4u::LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
  }
}

void DMapFileParserTest::testErrorInDmapFile() {
  std::string incorrect_dmap_file = "invalid.dmap";
  mtca4u::DMapFileParser fileParser;

  BOOST_CHECK_THROW(fileParser.parse(incorrect_dmap_file), mtca4u::LibMapException);
  try {
    fileParser.parse(incorrect_dmap_file);
  }
  catch (mtca4u::LibMapException& dMapFileParserException) {
    std::cout << dMapFileParserException;
    BOOST_CHECK(dMapFileParserException.getID() ==
        mtca4u::LibMapException::EX_DMAP_FILE_PARSE_ERROR);
  }
  
  BOOST_CHECK_THROW(fileParser.parse("badLoadlib.dmap"), mtca4u::LibMapException);
  BOOST_CHECK_THROW(fileParser.parse("badLoadlib2.dmap"), mtca4u::LibMapException);
  BOOST_CHECK_THROW(fileParser.parse("unkownKey.dmap"), mtca4u::LibMapException);

}

void DMapFileParserTest::testNoDataInDmapFile() {
  std::string empty_dmap_file = "empty.dmap";
  mtca4u::DMapFileParser fileParser;

  BOOST_CHECK_THROW(fileParser.parse(empty_dmap_file), mtca4u::LibMapException);
  try {
    fileParser.parse(empty_dmap_file);
  }
  catch (mtca4u::LibMapException& dMapFileParserException) {
    std::cout << dMapFileParserException;
    BOOST_CHECK(dMapFileParserException.getID() ==
        mtca4u::LibMapException::EX_NO_DMAP_DATA);
  }
}

void DMapFileParserTest::testParseFile() {
  std::string file_path = "valid.dmap";
  mtca4u::DMapFileParser fileParser;
  boost::shared_ptr<mtca4u::DeviceInfoMap> mapFilePtr = fileParser.parse(file_path);

  mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
  mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;
  mtca4u::DeviceInfoMap::DeviceInfo deviceInfo3;

  std::string absPathToDmap = mtca4u::parserUtilities::convertToAbsolutePath("valid.dmap");
  std::string absPathToDmapDir = mtca4u::parserUtilities::getCurrentWorkingDirectory();

  populateDummyDeviceInfo(deviceInfo1, absPathToDmap, "card1", "/dev/dev1",
      mtca4u::parserUtilities::concatenatePaths(absPathToDmapDir, "goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(deviceInfo2, absPathToDmap, "card2", "/dev/dev2",
      mtca4u::parserUtilities::concatenatePaths(absPathToDmapDir, "./goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(deviceInfo3, absPathToDmap, "card3", "/dev/dev3",
      mtca4u::parserUtilities::getCurrentWorkingDirectory()+"goodMapFile_withoutModules.map");

  deviceInfo1.dmapFileLineNumber = 6;
  deviceInfo2.dmapFileLineNumber = 7;
  deviceInfo3.dmapFileLineNumber = 8;

  // we use require here so it is safe to increase and dereference the iterator below
  BOOST_REQUIRE( mapFilePtr->getSize() == 3);

  mtca4u::DeviceInfoMap::const_iterator it = mapFilePtr->begin();

  BOOST_CHECK( compareDeviceInfos(deviceInfo1, *(it++)) == true);
  BOOST_CHECK( compareDeviceInfos(deviceInfo2, *(it++)) == true);
  BOOST_CHECK( compareDeviceInfos(deviceInfo3, *(it++)) == true);

  auto pluginLibraries = mapFilePtr->getPluginLibraries();
  
  BOOST_CHECK( pluginLibraries.size() == 2 );
  BOOST_CHECK( pluginLibraries[0]  == "libMyLib.so" );
  BOOST_CHECK( pluginLibraries[1] == "libAnotherLib.so" );
}
