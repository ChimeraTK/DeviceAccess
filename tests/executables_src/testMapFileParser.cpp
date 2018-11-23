///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include <sstream>

#include "MapFileParser.h"
#include "Exception.h"
#include "helperFunctions.h"

namespace ChimeraTK{
  using namespace ChimeraTK;
}
using namespace boost::unit_test_framework;

class MapFileParserTest {
  public:
    void testFileDoesNotExist();
    void testInvalidMetadata();
    void testMandatoryRegisterFieldMissing();
    void testIncorrectRegisterWidth();
    void testFracBits();
    void testGoodMapFileParse();
    void testGoodMappFileParse();
    void testMixedMapFileParse();
    void testSplitStringAtLastDot();
    void testBadMappFileParse();
};
class MapFileParserTestSuite : public test_suite{
  public:
    MapFileParserTestSuite():test_suite("mapFileParser class test suite"){

      boost::shared_ptr<MapFileParserTest>
      mapFileParserTestPtr(new MapFileParserTest());

      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testFileDoesNotExist,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testInvalidMetadata,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE( &MapFileParserTest::testMandatoryRegisterFieldMissing,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testIncorrectRegisterWidth,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testFracBits,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testGoodMapFileParse,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testGoodMappFileParse,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testMixedMapFileParse,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testSplitStringAtLastDot,
          mapFileParserTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&MapFileParserTest::testBadMappFileParse,
          mapFileParserTestPtr) );
    }
};

bool init_unit_test(){
  framework::master_test_suite().p_name.value = "mapFileParser test suite";
  framework::master_test_suite().add(new MapFileParserTestSuite());

  return true;
}

void MapFileParserTest::testFileDoesNotExist(){
  ChimeraTK::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("NonexistentFile.map"),
      ChimeraTK::logic_error);
}

void MapFileParserTest::testInvalidMetadata(){
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("invalid_metadata.map"),
      ChimeraTK::logic_error);
}

void MapFileParserTest::testMandatoryRegisterFieldMissing () {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("MandatoryRegisterfIeldMissing.map"),
      ChimeraTK::logic_error);
}

void MapFileParserTest::testIncorrectRegisterWidth () {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth.map"),
      ChimeraTK::logic_error);
}

void MapFileParserTest::testFracBits () {
  ChimeraTK::MapFileParser map_file_parser1;
  ChimeraTK::MapFileParser map_file_parser2;
  BOOST_CHECK_THROW(map_file_parser1.parse("IncorrectFracBits1.map"),
      ChimeraTK::logic_error);
  BOOST_CHECK_THROW(map_file_parser2.parse("IncorrectFracBits2.map"),
      ChimeraTK::logic_error);
}

void MapFileParserTest::testGoodMapFileParse () {
  ChimeraTK::MapFileParser map_file_parser;
  boost::shared_ptr<ChimeraTK::RegisterInfoMap> ptrmapFile =
      map_file_parser.parse("goodMapFile_withoutModules.map");

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "2.5");

  /* TODO: remove default assignments to unused fields in the parse
   * function and
   * move it all to the constructor */
  ChimeraTK::RegisterInfoMap::RegisterInfo RegisterInfoent1("WORD_FIRMWARE", 0x00000001,
      0x00000000, 0x00000004,
      0x0, 32, 0, true, 5);
  ChimeraTK::RegisterInfoMap::RegisterInfo RegisterInfoent2("WORD_COMPILATION", 0x00000001,
      0x00000004, 0x00000004,
      0x00000000, 32, 0, true, 6);
  ChimeraTK::RegisterInfoMap::RegisterInfo RegisterInfoent3("WORD_STATUS", 0x00000001,
      0x00000008, 0x00000004,
      0x00000000, 32, 0, true, 7);
  ChimeraTK::RegisterInfoMap::RegisterInfo RegisterInfoent4("WORD_USER1", 0x00000001,
      0x0000000C, 0x00000004,
      0x00000000, 32, 0, true, 8);
  ChimeraTK::RegisterInfoMap::RegisterInfo RegisterInfoent5("WORD_USER2", 0x00000001,
      0x00000010, 0x00000004,
      0x00000000, 32, 0, 0, 9);

  ChimeraTK::RegisterInfoMap::RegisterInfo* ptrList[5];
  ptrList[0] = &RegisterInfoent1;
  ptrList[1] = &RegisterInfoent2;
  ptrList[2] = &RegisterInfoent3;
  ptrList[3] = &RegisterInfoent4;
  ptrList[4] = &RegisterInfoent5;

  int index;
  ChimeraTK::RegisterInfoMap::iterator it;
  for( it = ptrmapFile->begin(), index = 0;
      it != ptrmapFile->end(); ++it, ++index){
    BOOST_CHECK((compareRegisterInfoents(*ptrList[index], *it)) == true);
  }
}

void MapFileParserTest::testGoodMappFileParse () {
  ChimeraTK::MapFileParser map_file_parser;
  boost::shared_ptr<ChimeraTK::RegisterInfoMap> ptrmapFile =
      map_file_parser.parse("goodMapFile.map");

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "2.5");

  std::vector< ChimeraTK::RegisterInfoMap::RegisterInfo > RegisterInfoents(13);

  RegisterInfoents[0] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_FIRMWARE", 0x01, 0x0, 0x04, 0x0,
      32, 0, true, 5, "BOARD");
  RegisterInfoents[1] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_COMPILATION", 0x01, 0x04, 0x04, 0x0,
      32, 0, true, 6, "BOARD");
  RegisterInfoents[2] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_STATUS", 0x01, 0x08, 0x04, 0x01,
      32, 0, true, 7, "APP0");
  RegisterInfoents[3] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_SCRATCH", 0x01, 0x08, 0x04, 0x01,
      16, 0, true, 8, "APP0");
  RegisterInfoents[4] = ChimeraTK::RegisterInfoMap::RegisterInfo("MODULE0", 0x03, 0x10, 0x0C, 0x01,
      32, 0, true, 9, "APP0");
  RegisterInfoents[5] = ChimeraTK::RegisterInfoMap::RegisterInfo("MODULE1", 0x03, 0x20, 0x0C, 0x01,
      32, 0, true, 10, "APP0");
  RegisterInfoents[6] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER1", 0x01, 0x10, 0x04, 0x01,
      16, 3, true, 14, "MODULE0");
  RegisterInfoents[7] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER2", 0x01, 0x14, 0x04, 0x01,
      18, 5, false, 15, "MODULE0");
  RegisterInfoents[8] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER3", 0x01, 0x18, 0x04, 0x01,
      18, 5, false, 16, "MODULE0");
  RegisterInfoents[9] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER1", 0x01, 0x20, 0x04, 0x01,
      16, 3, true, 17, "MODULE1");
  RegisterInfoents[10] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER2", 0x01, 0x24, 0x04, 0x01,
      18, 5, false, 18, "MODULE1");
  RegisterInfoents[11] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER3", 0x01, 0x28, 0x04, 0x01,
      18, 5, false, 19, "MODULE1");
  RegisterInfoents[12] = ChimeraTK::RegisterInfoMap::RegisterInfo("REGISTER", 0x01, 0x00, 0x04, 0x02,
      32, 0, true, 22, "MODULE.NAME.WITH.DOTS");

  ChimeraTK::RegisterInfoMap::const_iterator mapIter;
  std::vector<ChimeraTK::RegisterInfoMap::RegisterInfo>::const_iterator elementsIter;
  for( mapIter = ptrmapFile->begin(), elementsIter = RegisterInfoents.begin();
      mapIter != ptrmapFile->end() && elementsIter != RegisterInfoents.end();
      ++mapIter, ++elementsIter){
    std::stringstream message;
    message << "Failed comparison on Register '" << (*elementsIter).name
        << "', module '" << (elementsIter->module) << "'";
    BOOST_CHECK_MESSAGE( compareRegisterInfoents(*mapIter, *elementsIter) == true,
        message.str());
  }

}

void MapFileParserTest::testMixedMapFileParse () {
  ChimeraTK::MapFileParser map_file_parser;
  boost::shared_ptr<ChimeraTK::RegisterInfoMap> ptrmapFile =
      map_file_parser.parse("mixedMapFile.map");

  std::vector< ChimeraTK::RegisterInfoMap::RegisterInfo > RegisterInfoents(4);

  RegisterInfoents[0] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_FIRMWARE_ID", 0x01, 0x0, 0x04, 0x0,
      32, 0, true, 4);
  RegisterInfoents[1] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER", 0x01, 0x4, 0x04, 0x0,
      32, 0, true, 5);
  RegisterInfoents[2] = ChimeraTK::RegisterInfoMap::RegisterInfo("MODULE_ID", 0x01, 0x0, 0x04, 0x1,
      32, 0, true, 6, "APP0");
  RegisterInfoents[3] = ChimeraTK::RegisterInfoMap::RegisterInfo("WORD_USER", 0x03, 0x4, 0x0C, 0x1,
      18, 3, false, 7, "APP0");

  ChimeraTK::RegisterInfoMap::const_iterator mapIter;
  std::vector<ChimeraTK::RegisterInfoMap::RegisterInfo>::const_iterator elementsIter;
  for( mapIter = ptrmapFile->begin(), elementsIter = RegisterInfoents.begin();
      mapIter != ptrmapFile->end() && elementsIter != RegisterInfoents.end();
      ++mapIter, ++elementsIter){
    std::stringstream message;
    message << "Failed comparison on Register '" << (*elementsIter).name
        << "', module '" << (elementsIter->module) << "'";
    BOOST_CHECK_MESSAGE( compareRegisterInfoents(*mapIter, *elementsIter) == true,
        message.str());
  }
  //Ugly, just to get Code Coverage.
  std::stringstream message;
  message << "Failure";
  mapIter = ptrmapFile->begin();
  elementsIter = RegisterInfoents.begin();
  ++elementsIter;
  BOOST_CHECK_MESSAGE(compareRegisterInfoents(*mapIter,*elementsIter) == false, message.str());
}

void MapFileParserTest::testSplitStringAtLastDot(){
  std::string simple("SIMPLE_REGISTER");
  std::string normal("MODULE.REGISTER");
  std::string withDots("MODULE.NAME.WITH.DOTS.REGISTER");
  std::string stillRegister(".STILL_REGISTER");
  std::string emptyRegister("MODULE.");
  std::string justDot(".");

  std::pair<std::string, std::string> stringPair =
      ChimeraTK::MapFileParser::splitStringAtLastDot(simple);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second == simple );

  stringPair = ChimeraTK::MapFileParser::splitStringAtLastDot(normal);
  BOOST_CHECK( stringPair.first == "MODULE" );
  BOOST_CHECK( stringPair.second == "REGISTER" );

  stringPair = ChimeraTK::MapFileParser::splitStringAtLastDot(withDots);
  BOOST_CHECK( stringPair.first == "MODULE.NAME.WITH.DOTS" );
  BOOST_CHECK( stringPair.second == "REGISTER" );

  stringPair = ChimeraTK::MapFileParser::splitStringAtLastDot(stillRegister);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second == "STILL_REGISTER" );

  stringPair = ChimeraTK::MapFileParser::splitStringAtLastDot(emptyRegister);
  BOOST_CHECK( stringPair.first == "MODULE" );
  BOOST_CHECK( stringPair.second.empty() );

  stringPair = ChimeraTK::MapFileParser::splitStringAtLastDot(justDot);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second.empty() );

}

void MapFileParserTest::testBadMappFileParse(){
  ChimeraTK::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("badMapFile.map"),
      ChimeraTK::logic_error);
}
