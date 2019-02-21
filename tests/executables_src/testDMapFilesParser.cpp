///@todo FIXME My dynamic init header is a hack. Change the test to use
///BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include "DMapFilesParser.h"
#include "Exception.h"
#include "Utilities.h"
#include "helperFunctions.h"
#include "parserUtilities.h"

#include <boost/bind.hpp>

using namespace boost::unit_test_framework;
namespace ChimeraTK {
using namespace ChimeraTK;
}

class DMapFilesParserTest {
public:
  /** Test the parsing of a file and check the DeviceInfos.
   *  If the path is not empty it must end with '/'.
   */
  void testParseFile(std::string pathToDmapFile);
  void testParseEmptyDmapFile();
  void testParseNonExistentDmapFile(std::string dmapFile);
  void testGetMapFile();
  void testGetRegisterInfo();
  void testGetDMapFileSize();
  void testCheckParsedInInfo();
  void testOverloadedStreamOperator();
  void testIteratorBeginEnd();
  void testParsedirInvalidDir();
  void testParseEmptyDirectory();
  void testParseDirectoryWithBlankDMap();
  void testParseDirWithGoodDmaps();
  void testParseDirs();
  void testConstructor();
  void testMapException();
};

class DMapFilesParserTestSuite : public test_suite {
public:
  DMapFilesParserTestSuite() : test_suite("dMapFilesParser class test suite") {
    boost::shared_ptr<DMapFilesParserTest> DMapFilesParserTestPtr(
        new DMapFilesParserTest);

    // test parsing of dmap file in the current directory
    add(BOOST_TEST_CASE(boost::bind(&DMapFilesParserTest::testParseFile,
                                    DMapFilesParserTestPtr, "")));
    add(BOOST_TEST_CASE(boost::bind(&DMapFilesParserTest::testParseFile,
                                    DMapFilesParserTestPtr, "dMapDir/")));
    add(BOOST_TEST_CASE(boost::bind(
        &DMapFilesParserTest::testParseFile, DMapFilesParserTestPtr,
        ChimeraTK::parserUtilities::getCurrentWorkingDirectory() + "/")));
    // test with an empty, existing file
    add(BOOST_CLASS_TEST_CASE(&DMapFilesParserTest::testParseEmptyDmapFile,
                              DMapFilesParserTestPtr));
    // test with a non-existing file
    add(BOOST_TEST_CASE(
        boost::bind(&DMapFilesParserTest::testParseNonExistentDmapFile,
                    DMapFilesParserTestPtr, "notExisting.dmap")));
    // special case: file in the root directory. Cannot be there in the test, an
    // probably also not in real life.
    add(BOOST_TEST_CASE(
        boost::bind(&DMapFilesParserTest::testParseNonExistentDmapFile,
                    DMapFilesParserTestPtr, "/some.dmap")));

    test_case *testGetMapFile = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testGetMapFile, DMapFilesParserTestPtr);
    test_case *testRegisterInfo = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testGetRegisterInfo, DMapFilesParserTestPtr);
    test_case *testGetDMapFileSize = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testGetDMapFileSize, DMapFilesParserTestPtr);
    test_case *testCheckParsedInInfo = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testCheckParsedInInfo, DMapFilesParserTestPtr);
    test_case *testOverloadedStreamOperator = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testOverloadedStreamOperator,
        DMapFilesParserTestPtr);
    test_case *testIteratorBeginEnd = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testIteratorBeginEnd, DMapFilesParserTestPtr);

    test_case *testParsedirInvalidDir = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testParsedirInvalidDir, DMapFilesParserTestPtr);
    test_case *testParseEmptyDirectory = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testParseEmptyDirectory, DMapFilesParserTestPtr);
    test_case *testParseDirectoryWithBlankDMap = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testParseDirectoryWithBlankDMap,
        DMapFilesParserTestPtr);
    test_case *testParseDirWithGoodDmaps =
        BOOST_CLASS_TEST_CASE(&DMapFilesParserTest::testParseDirWithGoodDmaps,
                              DMapFilesParserTestPtr);
    test_case *testParseDirs = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testParseDirs, DMapFilesParserTestPtr);

    test_case *testConstructor = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testConstructor, DMapFilesParserTestPtr);
    test_case *testMapException = BOOST_CLASS_TEST_CASE(
        &DMapFilesParserTest::testMapException, DMapFilesParserTestPtr);

    add(testGetMapFile);
    add(testRegisterInfo);
    add(testGetDMapFileSize);
    add(testCheckParsedInInfo);
    add(testOverloadedStreamOperator);
    add(testIteratorBeginEnd);
    add(testParsedirInvalidDir);
    add(testParseEmptyDirectory);
    add(testParseDirectoryWithBlankDMap);
    add(testParseDirWithGoodDmaps);
    add(testParseDirs);
    add(testConstructor);
    add(testMapException);
  }
};

bool init_unit_test() {
  framework::master_test_suite().p_name.value =
      "dMapFilesParser class test suite";
  framework::master_test_suite().add(new DMapFilesParserTestSuite());

  return true;
}

void DMapFilesParserTest::testParseFile(std::string pathToDmapFile) {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dmap_file = pathToDmapFile + "valid.dmap";
  std::string path_to_map_file1 = "goodMapFile_withoutModules.map";
  std::string path_to_map_file2 = "./goodMapFile_withoutModules.map";
  std::string path_to_map_file3 =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() +
      "goodMapFile_withoutModules.map";
  filesParser.parse_file(path_to_dmap_file);
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo3;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo4;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo5;

  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo3;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo4;

  std::string absolutePathToDMapFile =
      ChimeraTK::parserUtilities::convertToAbsolutePath(path_to_dmap_file);
  std::string currentWorkingDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory();
  std::string absolutePathToDMapDir =
      ChimeraTK::parserUtilities::concatenatePaths(currentWorkingDir,
                                                   pathToDmapFile);

  populateDummyDeviceInfo(expectedDeviceInfo1, absolutePathToDMapFile, "card1",
                          "/dev/dev1",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absolutePathToDMapDir, path_to_map_file1));
  populateDummyDeviceInfo(expectedDeviceInfo2, absolutePathToDMapFile, "card2",
                          "/dev/dev2",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absolutePathToDMapDir, path_to_map_file2));
  populateDummyDeviceInfo(expectedDeviceInfo3, absolutePathToDMapFile, "card3",
                          "/dev/dev3",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absolutePathToDMapDir, path_to_map_file3));
  populateDummyDeviceInfo(
      expectedDeviceInfo4, absolutePathToDMapFile, "card4",
      "(pci:mtcadummys0?map=goodMapFile_withoutModules.map)", "");

  expectedDeviceInfo1.dmapFileLineNumber = 6;
  expectedDeviceInfo2.dmapFileLineNumber = 7;
  expectedDeviceInfo3.dmapFileLineNumber = 8;
  expectedDeviceInfo4.dmapFileLineNumber = 9;

  filesParser.getdMapFileElem(0, reterievedDeviceInfo1);
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1, reterievedDeviceInfo1) ==
              true);

  filesParser.getdMapFileElem(1, reterievedDeviceInfo2);
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2, reterievedDeviceInfo2) ==
              true);

  filesParser.getdMapFileElem(2, reterievedDeviceInfo3);
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo3, reterievedDeviceInfo3) ==
              true);

  filesParser.getdMapFileElem(3, reterievedDeviceInfo4);
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo4, reterievedDeviceInfo4) ==
              true);

  BOOST_CHECK_THROW(filesParser.getdMapFileElem(4, reterievedDeviceInfo5),
                    ChimeraTK::logic_error);

  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo6 =
      filesParser.getdMapFileElem("card2");

  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2, reterievedDeviceInfo6) ==
              true);

  BOOST_CHECK_THROW(filesParser.getdMapFileElem("card_not_present"),
                    ChimeraTK::logic_error);

  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo7;
  filesParser.getdMapFileElem("card2", reterievedDeviceInfo7);
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2, reterievedDeviceInfo7) ==
              true);
}

void DMapFilesParserTest::testParseEmptyDmapFile() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string dmapFileName("empty.dmap");

  BOOST_CHECK_THROW(filesParser.parse_file(dmapFileName),
                    ChimeraTK::logic_error);
}

void DMapFilesParserTest::testParseNonExistentDmapFile(std::string dmapFile) {
  ChimeraTK::DMapFilesParser filesParser;

  BOOST_CHECK_THROW(filesParser.parse_file(dmapFile), ChimeraTK::logic_error);
}

void DMapFilesParserTest::testGetMapFile() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dmap_file = "dMapDir/valid.dmap";
  filesParser.parse_file(path_to_dmap_file);

  boost::shared_ptr<ChimeraTK::RegisterInfoMap> map_file_for_card1;
  boost::shared_ptr<ChimeraTK::RegisterInfoMap> map_file_for_card3;

  map_file_for_card1 = filesParser.getMapFile("card1");
  map_file_for_card3 = filesParser.getMapFile("card3");

  // Card 1 elements
  ChimeraTK::RegisterInfoMap::RegisterInfo card1_RegisterInfoent1(
      "WORD_FIRMWARE", 0x00000001, 0x00000000, 0x00000004, 0x0, 32, 0, true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card1_RegisterInfoent2(
      "WORD_COMPILATION", 0x00000001, 0x00000004, 0x00000004, 0x00000000, 32, 0,
      true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card1_RegisterInfoent3(
      "WORD_STATUS", 0x00000001, 0x00000008, 0x00000004, 0x00000000, 32, 0,
      true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card1_RegisterInfoent4(
      "WORD_USER1", 0x00000001, 0x0000000C, 0x00000004, 0x00000000, 32, 0,
      true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card1_RegisterInfoent5(
      "WORD_USER2", 0x00000001, 0x00000010, 0x00000004, 0x00000000, 32, 0,
      false);

  ChimeraTK::RegisterInfoMap::RegisterInfo *ptrList[5];
  ptrList[0] = &card1_RegisterInfoent1;
  ptrList[1] = &card1_RegisterInfoent2;
  ptrList[2] = &card1_RegisterInfoent3;
  ptrList[3] = &card1_RegisterInfoent4;
  ptrList[4] = &card1_RegisterInfoent5;
  int index;
  ChimeraTK::RegisterInfoMap::iterator it;
  for (it = map_file_for_card1->begin(), index = 0;
       it != map_file_for_card1->end(); ++it, ++index) {
    BOOST_CHECK((compareRegisterInfoents(*ptrList[index], *it)) == true);
  }

  // Card 3 elements
  ChimeraTK::RegisterInfoMap::RegisterInfo card3_RegisterInfoent1(
      "WORD_FIRMWARE", 0x00000001, 0x00000000, 0x00000004, 0x0, 32, 0, true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card3_RegisterInfoent2(
      "WORD_COMPILATION", 0x00000001, 0x00000004, 0x00000004, 0x00000000, 32, 0,
      true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card3_RegisterInfoent3(
      "WORD_STATUS", 0x00000001, 0x00000008, 0x00000004, 0x00000000, 32, 0,
      true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card3_RegisterInfoent4(
      "WORD_USER1", 0x00000001, 0x0000000C, 0x00000004, 0x00000000, 32, 0,
      true);
  ChimeraTK::RegisterInfoMap::RegisterInfo card3_RegisterInfoent5(
      "WORD_USER2", 0x00000001, 0x00000010, 0x00000004, 0x00000000, 32, 0,
      false);

  ptrList[0] = &card3_RegisterInfoent1;
  ptrList[1] = &card3_RegisterInfoent2;
  ptrList[2] = &card3_RegisterInfoent3;
  ptrList[3] = &card3_RegisterInfoent4;
  ptrList[4] = &card3_RegisterInfoent5;
  for (it = map_file_for_card3->begin(), index = 0;
       it != map_file_for_card3->end(); ++it, ++index) {
    BOOST_CHECK((compareRegisterInfoents(*ptrList[index], *it)) == true);
  }

  BOOST_CHECK_THROW(filesParser.getMapFile("card_unknown"),
                    ChimeraTK::logic_error);
}

void DMapFilesParserTest::testGetRegisterInfo() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dmap_file = "dMapDir/valid.dmap";
  filesParser.parse_file(path_to_dmap_file);

  std::string retrivedDeviceFileName;
  ChimeraTK::RegisterInfoMap::RegisterInfo retrivedRegisterInfo;
  ChimeraTK::RegisterInfoMap::RegisterInfo RegisterInfoent3(
      "WORD_STATUS", 0x00000001, 0x00000008, 0x00000004, 0x00000000, 32, 0,
      true);
  filesParser.getRegisterInfo("card1", "WORD_STATUS", retrivedDeviceFileName,
                              retrivedRegisterInfo);
  BOOST_CHECK(retrivedDeviceFileName == "/dev/dev1");
  BOOST_CHECK(compareRegisterInfoents(retrivedRegisterInfo, RegisterInfoent3) ==
              true);

  filesParser.getRegisterInfo("card3", "WORD_STATUS", retrivedDeviceFileName,
                              retrivedRegisterInfo);

  BOOST_CHECK(retrivedDeviceFileName == "/dev/dev3");
  BOOST_CHECK(compareRegisterInfoents(retrivedRegisterInfo, RegisterInfoent3) ==
              true);

  BOOST_CHECK_THROW(filesParser.getRegisterInfo("card_unknown", "WORD_STATUS",
                                                retrivedDeviceFileName,
                                                retrivedRegisterInfo),
                    ChimeraTK::logic_error);

  ChimeraTK::DMapFilesParser filesParser2;
  path_to_dmap_file = "dMapDir/oneDevice.dmap";
  filesParser2.parse_file(path_to_dmap_file);
  filesParser2.getRegisterInfo("", "WORD_STATUS", retrivedDeviceFileName,
                               retrivedRegisterInfo);
  BOOST_CHECK(retrivedDeviceFileName == "/dev/dev1");
  BOOST_CHECK(compareRegisterInfoents(retrivedRegisterInfo, RegisterInfoent3) ==
              true);

  uint32_t retrivedElemNr;
  uint32_t retrivedOffset;
  uint32_t retrivedRegSize;
  uint32_t retrivedRegBar;

  filesParser.getRegisterInfo("card2", "WORD_STATUS", retrivedDeviceFileName,
                              retrivedElemNr, retrivedOffset, retrivedRegSize,
                              retrivedRegBar);
  BOOST_CHECK(retrivedDeviceFileName == "/dev/dev2");
  BOOST_CHECK(retrivedElemNr == 1);
  BOOST_CHECK(retrivedOffset == 0x00000008);
  BOOST_CHECK(retrivedRegSize == 0x00000004);
  BOOST_CHECK(retrivedRegBar == 0x00000000);

  filesParser2.getRegisterInfo("", "WORD_STATUS", retrivedDeviceFileName,
                               retrivedElemNr, retrivedOffset, retrivedRegSize,
                               retrivedRegBar);
  BOOST_CHECK(retrivedDeviceFileName == "/dev/dev1");
  BOOST_CHECK(retrivedElemNr == 1);
  BOOST_CHECK(retrivedOffset == 0x00000008);
  BOOST_CHECK(retrivedRegSize == 0x00000004);
  BOOST_CHECK(retrivedRegBar == 0x00000000);

  BOOST_CHECK_THROW(filesParser.getRegisterInfo(
                        "unknown_card", "WORD_STATUS", retrivedDeviceFileName,
                        retrivedElemNr, retrivedOffset, retrivedRegSize,
                        retrivedRegBar),
                    ChimeraTK::logic_error);
}

void DMapFilesParserTest::testGetDMapFileSize() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dmap_file = "dMapDir/valid.dmap";
  filesParser.parse_file(path_to_dmap_file);

  BOOST_CHECK(filesParser.getdMapFileSize() == 4);
}

void DMapFilesParserTest::testCheckParsedInInfo() {
  ChimeraTK::DMapFilesParser filesParser;
  ChimeraTK::DMapFilesParser filesParser1;
  std::string path_to_dmap_file = "dMapDir/NonUniqueCardName.dmap";
  filesParser.parse_file(path_to_dmap_file);
  filesParser1.parse_file("dMapDir/oneDevice.dmap");

  ChimeraTK::DeviceInfoMap::ErrorList dmap_err_list;
  ChimeraTK::RegisterInfoMap::ErrorList map_err_list;

  bool returnValue = filesParser1.check(
      ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
      ChimeraTK::RegisterInfoMap::ErrorList::ErrorElem::WARNING, dmap_err_list,
      map_err_list);
  BOOST_CHECK(returnValue == true);

  bool status = filesParser.check(
      ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
      ChimeraTK::RegisterInfoMap::ErrorList::ErrorElem::WARNING, dmap_err_list,
      map_err_list);

  BOOST_CHECK(status == false);
  int numberOfIncorrectLinesInFile = dmap_err_list._errors.size();
  BOOST_CHECK(numberOfIncorrectLinesInFile == 1);
  std::list<ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem>::iterator
      errorIterator = dmap_err_list._errors.begin();
  BOOST_CHECK((errorIterator->_errorDevice1.deviceName ==
               errorIterator->_errorDevice2.deviceName));

  BOOST_CHECK(map_err_list.errors.size() == 2);
  std::list<ChimeraTK::RegisterInfoMap::ErrorList::ErrorElem>::iterator
      mapErrIt;
  mapErrIt = map_err_list.errors.begin();

  bool areNonUniqueRegistersPresent =
      ((mapErrIt->_errorRegister1.name == mapErrIt->_errorRegister2.name) &&
       ((mapErrIt->_errorRegister1.address !=
         mapErrIt->_errorRegister2.address) ||
        (mapErrIt->_errorRegister1.bar != mapErrIt->_errorRegister2.bar) ||
        (mapErrIt->_errorRegister1.nElements !=
         mapErrIt->_errorRegister2.nElements) ||
        (mapErrIt->_errorRegister1.nBytes !=
         mapErrIt->_errorRegister2.nBytes)));
  BOOST_CHECK(areNonUniqueRegistersPresent);
}

void DMapFilesParserTest::testOverloadedStreamOperator() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dmap_file = "dMapDir/valid.dmap";
  std::string absPathToDMapFile =
      ChimeraTK::parserUtilities::convertToAbsolutePath(path_to_dmap_file);
  std::string absPathToDMapDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() + "dMapDir";
  filesParser.parse_file(path_to_dmap_file);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo3;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo4;

  populateDummyDeviceInfo(
      deviceInfo1, absPathToDMapFile, "card1", "/dev/dev1",
      ChimeraTK::parserUtilities::concatenatePaths(
          absPathToDMapDir, "goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(
      deviceInfo2, absPathToDMapFile, "card2", "/dev/dev2",
      ChimeraTK::parserUtilities::concatenatePaths(
          absPathToDMapDir, "./goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(
      deviceInfo3, absPathToDMapFile, "card3", "/dev/dev3",
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() +
          "goodMapFile_withoutModules.map");
  populateDummyDeviceInfo(
      deviceInfo4, absPathToDMapFile, "card4",
      "(pci:mtcadummys0?map=goodMapFile_withoutModules.map)", "");

  deviceInfo1.dmapFileLineNumber = 6;
  deviceInfo2.dmapFileLineNumber = 7;
  deviceInfo3.dmapFileLineNumber = 8;
  deviceInfo3.dmapFileLineNumber = 9;

  std::stringstream expected_file_stream;
  expected_file_stream << deviceInfo1 << std::endl;
  expected_file_stream << deviceInfo2 << std::endl;
  expected_file_stream << deviceInfo3 << std::endl;
  expected_file_stream << deviceInfo4 << std::endl;

  std::stringstream actual_file_stream;
  actual_file_stream << filesParser;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFilesParserTest::testIteratorBeginEnd() {
  ChimeraTK::DMapFilesParser filesParser;
  ChimeraTK::DMapFilesParser const &constfilesParser = filesParser;
  std::string path_to_dmap_file = "dMapDir/valid.dmap";
  std::string absPathToDMapFile =
      ChimeraTK::parserUtilities::convertToAbsolutePath(path_to_dmap_file);
  std::string absPathToDMapDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() + "dMapDir";

  filesParser.parse_file(path_to_dmap_file);

  std::string currentWrkingDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory();

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo3;

  populateDummyDeviceInfo(
      deviceInfo1, absPathToDMapFile, "card1", "/dev/dev1",
      ChimeraTK::parserUtilities::concatenatePaths(
          absPathToDMapDir, "goodMapFile_withoutModules.map"));
  populateDummyDeviceInfo(
      deviceInfo2, absPathToDMapFile, "card2", "/dev/dev2",
      ChimeraTK::parserUtilities::concatenatePaths(
          absPathToDMapDir, "./goodMapFile_withoutModules.map"));
  // the third path is absolute, does not change with the location of the dmap
  // file
  populateDummyDeviceInfo(
      deviceInfo3, absPathToDMapFile, "card3", "/dev/dev3",
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() +
          "goodMapFile_withoutModules.map");

  deviceInfo1.dmapFileLineNumber = 6;
  deviceInfo2.dmapFileLineNumber = 7;
  deviceInfo3.dmapFileLineNumber = 8;

  ChimeraTK::DeviceInfoMap::DeviceInfo *tmpArray1[3];
  tmpArray1[0] = &deviceInfo1;
  tmpArray1[1] = &deviceInfo2;
  tmpArray1[2] = &deviceInfo3;

  std::string s1 = currentWrkingDir + "dMapDir/goodMapFile_withoutModules.map";
  std::string s2 =
      currentWrkingDir + "dMapDir/./goodMapFile_withoutModules.map";
  // the third path is absolute, does not change with the location of the dmap
  // file
  std::string s3 = currentWrkingDir + "goodMapFile_withoutModules.map";
  std::string *tmpArray2[3];
  tmpArray2[0] = &s1;
  tmpArray2[1] = &s2;
  tmpArray2[2] = &s3;

  std::vector<std::pair<ChimeraTK::DeviceInfoMap::DeviceInfo,
                        ChimeraTK::RegisterInfoMapPointer>>::iterator iter;
  std::vector<std::pair<ChimeraTK::DeviceInfoMap::DeviceInfo,
                        ChimeraTK::RegisterInfoMapPointer>>::const_iterator
      const_iter;
  uint8_t i;
  for (iter = filesParser.begin(), i = 0;
       (iter != filesParser.end()) && (i < 3); ++i, ++iter) {
    BOOST_CHECK(compareDeviceInfos(*tmpArray1[i], (*iter).first) == true);
    BOOST_CHECK(*tmpArray2[i] == (*iter).second->getMapFileName());
  }
  for (const_iter = constfilesParser.begin(), i = 0;
       (const_iter != constfilesParser.end()) && (i < 3); ++i, ++const_iter) {
    BOOST_CHECK(compareDeviceInfos(*tmpArray1[i], (*const_iter).first) == true);
    BOOST_CHECK(*tmpArray2[i] == (*const_iter).second->getMapFileName());
  }
}

void DMapFilesParserTest::testParsedirInvalidDir() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dir = "NonExistentDir";

  BOOST_CHECK_THROW(filesParser.parse_dir(path_to_dir), ChimeraTK::logic_error);
}

void DMapFilesParserTest::testParseEmptyDirectory() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dir = "EmptyDir";

  BOOST_CHECK_THROW(filesParser.parse_dir(path_to_dir), ChimeraTK::logic_error);
}

void DMapFilesParserTest::testParseDirectoryWithBlankDMap() {
  ChimeraTK::DMapFilesParser filesParser;
  std::string path_to_dir = "./BlankFiles";

  BOOST_CHECK_THROW(filesParser.parse_dir(path_to_dir), ChimeraTK::logic_error);
}

void DMapFilesParserTest::testParseDirWithGoodDmaps() {
  ChimeraTK::DMapFilesParser filesParser;
  filesParser.parse_dir("./GoodDmapDir");

  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo3;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo4;

  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo3;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo4;

  std::string absPathToDmap1 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/first.dmap");
  std::string absPathToDmap2 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/second.dmap");
  std::string absPathToDmap3 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/second.dmap");
  std::string absPathToDmap4 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/first.dmap");

  std::string absPathToDmapDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() +
      "./GoodDmapDir";

  populateDummyDeviceInfo(expectedDeviceInfo1, absPathToDmap1, "card1",
                          "/dev/dev1",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile1.map"));
  populateDummyDeviceInfo(expectedDeviceInfo2, absPathToDmap2, "card2",
                          "/dev/dev2",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile2.map"));

  populateDummyDeviceInfo(expectedDeviceInfo3, absPathToDmap3, "card3",
                          "/dev/dev3",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile2.map"));
  populateDummyDeviceInfo(
      expectedDeviceInfo4, absPathToDmap4, "card4", "/dev/dev4",
      ChimeraTK::parserUtilities::concatenatePaths(
          absPathToDmapDir, "mtcadummy_withoutModules.map"));

  expectedDeviceInfo1.dmapFileLineNumber = 3;
  expectedDeviceInfo2.dmapFileLineNumber = 1;
  expectedDeviceInfo3.dmapFileLineNumber = 2;
  expectedDeviceInfo4.dmapFileLineNumber = 4;

  reterievedDeviceInfo1 = filesParser.getdMapFileElem("card1");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1, reterievedDeviceInfo1) ==
              true);

  reterievedDeviceInfo2 = filesParser.getdMapFileElem("card2");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2, reterievedDeviceInfo2) ==
              true);

  reterievedDeviceInfo3 = filesParser.getdMapFileElem("card3");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo3, reterievedDeviceInfo3) ==
              true);

  reterievedDeviceInfo4 = filesParser.getdMapFileElem("card4");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo4, reterievedDeviceInfo4) ==
              true);
}

void DMapFilesParserTest::testParseDirs() {
  std::vector<std::string> a;
  a.push_back("./GoodDmapDir");
  a.push_back("./BlankFiles");

  ChimeraTK::DMapFilesParser filesParser;
  filesParser.parse_dirs(a);

  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo2;

  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo2;

  std::string absPathToDmap1 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/first.dmap");
  std::string absPathToDmap2 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/second.dmap");

  std::string absPathToDmapDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() +
      "./GoodDmapDir";

  populateDummyDeviceInfo(expectedDeviceInfo1, absPathToDmap1, "card1",
                          "/dev/dev1",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile1.map"));
  populateDummyDeviceInfo(expectedDeviceInfo2, absPathToDmap2, "card2",
                          "/dev/dev2",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile2.map"));

  expectedDeviceInfo1.dmapFileLineNumber = 3;
  expectedDeviceInfo2.dmapFileLineNumber = 1;

  reterievedDeviceInfo1 = filesParser.getdMapFileElem("card1");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1, reterievedDeviceInfo1) ==
              true);

  reterievedDeviceInfo2 = filesParser.getdMapFileElem("card2");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2, reterievedDeviceInfo2) ==
              true);
}

inline void DMapFilesParserTest::testConstructor() {
  ChimeraTK::DMapFilesParser filesParser("./GoodDmapDir");

  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo reterievedDeviceInfo3;

  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo expectedDeviceInfo3;

  std::string absPathToDmap1 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/first.dmap");
  std::string absPathToDmap2 =
      ChimeraTK::parserUtilities::convertToAbsolutePath(
          "./GoodDmapDir/second.dmap");
  std::string absPathToDmapDir =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() +
      "./GoodDmapDir/";

  populateDummyDeviceInfo(expectedDeviceInfo1, absPathToDmap1, "card1",
                          "/dev/dev1",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile1.map"));
  populateDummyDeviceInfo(expectedDeviceInfo3, absPathToDmap2, "card3",
                          "/dev/dev3",
                          ChimeraTK::parserUtilities::concatenatePaths(
                              absPathToDmapDir, "./mapFile2.map"));

  expectedDeviceInfo1.dmapFileLineNumber = 3;
  expectedDeviceInfo3.dmapFileLineNumber = 2;

  reterievedDeviceInfo1 = filesParser.getdMapFileElem("card1");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1, reterievedDeviceInfo1) ==
              true);

  reterievedDeviceInfo3 = filesParser.getdMapFileElem("card3");
  BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo3, reterievedDeviceInfo3) ==
              true);
}

void DMapFilesParserTest::testMapException() {
  BOOST_CHECK_THROW(ChimeraTK::DMapFilesParser filesParser("./emptyMapFile"),
                    ChimeraTK::logic_error);
}
