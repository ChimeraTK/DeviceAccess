///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include "DMapFileParser.h"
#include "Exception.h"
#include "helperFunctions.h"
#include "parserUtilities.h"
#include "Utilities.h"

namespace ChimeraTK{
  using namespace ChimeraTK;
}
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

bool init_unit_test(){
  framework::master_test_suite().p_name.value =
      "DMapFileParser class test suite";
  framework::master_test_suite().add(new DMapFileParserTestSuite());

  return true;
}

void DMapFileParserTest::testFileNotFound() {
  std::string file_path = "../dummypath.dmap";
  ChimeraTK::DMapFileParser fileParser;

  BOOST_CHECK_THROW(fileParser.parse(file_path), ChimeraTK::logic_error);
}

void DMapFileParserTest::testErrorInDmapFile() {
  std::string incorrect_dmap_file = "invalid.dmap";
  ChimeraTK::DMapFileParser fileParser;

  BOOST_CHECK_THROW(fileParser.parse(incorrect_dmap_file), ChimeraTK::logic_error);
  
  BOOST_CHECK_THROW(fileParser.parse("badLoadlib.dmap"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileParser.parse("badLoadlib2.dmap"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileParser.parse("unkownKey.dmap"), ChimeraTK::logic_error);

}

void DMapFileParserTest::testNoDataInDmapFile() {
  std::string empty_dmap_file = "empty.dmap";
  ChimeraTK::DMapFileParser fileParser;

  BOOST_CHECK_THROW(fileParser.parse(empty_dmap_file), ChimeraTK::logic_error);
}

void DMapFileParserTest::testParseFile() {
  std::string file_path = "valid.dmap";
  ChimeraTK::DMapFileParser fileParser;
  boost::shared_ptr<ChimeraTK::DeviceInfoMap> mapFilePtr = fileParser.parse(file_path);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo3;

  std::string absPathToDmap = ChimeraTK::parserUtilities::convertToAbsolutePath("valid.dmap");
  std::string absPathToDmapDir = ChimeraTK::parserUtilities::getCurrentWorkingDirectory();

  populateDummyDeviceInfo(deviceInfo1, absPathToDmap, "card1", "/dev/dev1",
      ChimeraTK::parserUtilities::concatenatePaths(absPathToDmapDir, "goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(deviceInfo2, absPathToDmap, "card2", "/dev/dev2",
      ChimeraTK::parserUtilities::concatenatePaths(absPathToDmapDir, "./goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(deviceInfo3, absPathToDmap, "card3", "/dev/dev3",
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory()+"goodMapFile_withoutModules.map");

  deviceInfo1.dmapFileLineNumber = 6;
  deviceInfo2.dmapFileLineNumber = 7;
  deviceInfo3.dmapFileLineNumber = 8;

  // we use require here so it is safe to increase and dereference the iterator below
  BOOST_REQUIRE( mapFilePtr->getSize() == 3);

  ChimeraTK::DeviceInfoMap::const_iterator it = mapFilePtr->begin();

  BOOST_CHECK( compareDeviceInfos(deviceInfo1, *(it++)) == true);
  BOOST_CHECK( compareDeviceInfos(deviceInfo2, *(it++)) == true);
  BOOST_CHECK( compareDeviceInfos(deviceInfo3, *(it++)) == true);

  auto pluginLibraries = mapFilePtr->getPluginLibraries();
  
  BOOST_CHECK( pluginLibraries.size() == 2 );
  BOOST_CHECK( pluginLibraries[0]  == absPathToDmapDir+"libMyLib.so" );
  BOOST_CHECK( pluginLibraries[1] == "/system/libAnotherLib.so" );
}
