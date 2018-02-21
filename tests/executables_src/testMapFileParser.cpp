#include <boost/test/included/unit_test.hpp>
#include <sstream>

#include "MapFileParser.h"
#include "MapException.h"
#include "helperFunctions.h"

namespace mtca4u{
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

test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "mapFileParser test suite";
  framework::master_test_suite().add(new MapFileParserTestSuite());

  return NULL;
}

void MapFileParserTest::testFileDoesNotExist(){
  mtca4u::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("NonexistentFile.map"),
      mtca4u::MapFileException);
  try{
    fileparser.parse("NonexistentFile.map");
  } catch (mtca4u::MapFileException& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_CANNOT_OPEN_MAP_FILE);
  }
}

void MapFileParserTest::testInvalidMetadata(){
  mtca4u::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("invalid_metadata.map"),
      mtca4u::MapFileException);

  try{
    map_file_parser.parse("invalid_metadata.map");
  } catch (mtca4u::MapFileException &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_MAP_FILE_PARSE_ERROR);
  }
}

void MapFileParserTest::testMandatoryRegisterFieldMissing () {
  mtca4u::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("MandatoryRegisterfIeldMissing.map"),
      mtca4u::MapFileException);
  try{
    map_file_parser.parse("MandatoryRegisterfIeldMissing.map");
  }
  catch (mtca4u::MapFileException &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_MAP_FILE_PARSE_ERROR);
  }
}

void MapFileParserTest::testIncorrectRegisterWidth () {
  mtca4u::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth.map"),
      mtca4u::MapFileException);
  try{
    map_file_parser.parse("IncorrectRegisterWidth.map");
  }
  catch (mtca4u::MapFileException &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_MAP_FILE_PARSE_ERROR);
  }
}

void MapFileParserTest::testFracBits () {
  mtca4u::MapFileParser map_file_parser1;
  mtca4u::MapFileParser map_file_parser2;
  BOOST_CHECK_THROW(map_file_parser1.parse("IncorrectFracBits1.map"),
      mtca4u::MapFileException);
  BOOST_CHECK_THROW(map_file_parser2.parse("IncorrectFracBits2.map"),
      mtca4u::MapFileException);
  try{
    map_file_parser1.parse("IncorrectFracBits1.map");
  }
  catch (mtca4u::MapFileException &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_MAP_FILE_PARSE_ERROR);
  }

  try{
    map_file_parser1.parse("IncorrectFracBits2.map");
  }
  catch (mtca4u::MapFileException &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_MAP_FILE_PARSE_ERROR);
  }

}

void MapFileParserTest::testGoodMapFileParse () {
  mtca4u::MapFileParser map_file_parser;
  boost::shared_ptr<mtca4u::RegisterInfoMap> ptrmapFile =
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
  mtca4u::RegisterInfoMap::RegisterInfo RegisterInfoent1("WORD_FIRMWARE", 0x00000001,
      0x00000000, 0x00000004,
      0x0, 32, 0, true, 5);
  mtca4u::RegisterInfoMap::RegisterInfo RegisterInfoent2("WORD_COMPILATION", 0x00000001,
      0x00000004, 0x00000004,
      0x00000000, 32, 0, true, 6);
  mtca4u::RegisterInfoMap::RegisterInfo RegisterInfoent3("WORD_STATUS", 0x00000001,
      0x00000008, 0x00000004,
      0x00000000, 32, 0, true, 7);
  mtca4u::RegisterInfoMap::RegisterInfo RegisterInfoent4("WORD_USER1", 0x00000001,
      0x0000000C, 0x00000004,
      0x00000000, 32, 0, true, 8);
  mtca4u::RegisterInfoMap::RegisterInfo RegisterInfoent5("WORD_USER2", 0x00000001,
      0x00000010, 0x00000004,
      0x00000000, 32, 0, 0, 9);

  mtca4u::RegisterInfoMap::RegisterInfo* ptrList[5];
  ptrList[0] = &RegisterInfoent1;
  ptrList[1] = &RegisterInfoent2;
  ptrList[2] = &RegisterInfoent3;
  ptrList[3] = &RegisterInfoent4;
  ptrList[4] = &RegisterInfoent5;

  int index;
  mtca4u::RegisterInfoMap::iterator it;
  for( it = ptrmapFile->begin(), index = 0;
      it != ptrmapFile->end(); ++it, ++index){
    BOOST_CHECK((compareRegisterInfoents(*ptrList[index], *it)) == true);
  }
}

void MapFileParserTest::testGoodMappFileParse () {
  mtca4u::MapFileParser map_file_parser;
  boost::shared_ptr<mtca4u::RegisterInfoMap> ptrmapFile =
      map_file_parser.parse("goodMapFile.map");

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "2.5");

  std::vector< mtca4u::RegisterInfoMap::RegisterInfo > RegisterInfoents(11);

  RegisterInfoents[0] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_FIRMWARE", 0x01, 0x0, 0x04, 0x0,
      32, 0, true, 5, "BOARD");
  RegisterInfoents[1] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_COMPILATION", 0x01, 0x04, 0x04, 0x0,
      32, 0, true, 6, "BOARD");
  RegisterInfoents[2] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_STATUS", 0x01, 0x08, 0x04, 0x01,
      32, 0, true, 7, "APP0");
  RegisterInfoents[3] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_SCRATCH", 0x01, 0x08, 0x04, 0x01,
      16, 0, true, 8, "APP0");
  RegisterInfoents[4] = mtca4u::RegisterInfoMap::RegisterInfo("MODULE0", 0x02, 0x10, 0x08, 0x01,
      32, 0, true, 9, "APP0");
  RegisterInfoents[5] = mtca4u::RegisterInfoMap::RegisterInfo("MODULE1", 0x02, 0x20, 0x08, 0x01,
      32, 0, true, 10, "APP0");
  RegisterInfoents[6] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_USER1", 0x01, 0x10, 0x04, 0x01,
      16, 3, true, 14, "MODULE0");
  RegisterInfoents[7] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_USER2", 0x01, 0x14, 0x04, 0x01,
      18, 5, false, 15, "MODULE0");
  RegisterInfoents[8] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_USER1", 0x01, 0x20, 0x04, 0x01,
      16, 3, true, 16, "MODULE1");
  RegisterInfoents[9] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_USER2", 0x01, 0x24, 0x04, 0x01,
      18, 5, false, 17, "MODULE1");
  RegisterInfoents[10] = mtca4u::RegisterInfoMap::RegisterInfo("REGISTER", 0x01, 0x00, 0x04, 0x02,
      32, 0, true, 20, "MODULE.NAME.WITH.DOTS");

  mtca4u::RegisterInfoMap::const_iterator mapIter;
  std::vector<mtca4u::RegisterInfoMap::RegisterInfo>::const_iterator elementsIter;
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
  mtca4u::MapFileParser map_file_parser;
  boost::shared_ptr<mtca4u::RegisterInfoMap> ptrmapFile =
      map_file_parser.parse("mixedMapFile.map");

  std::vector< mtca4u::RegisterInfoMap::RegisterInfo > RegisterInfoents(4);

  RegisterInfoents[0] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_FIRMWARE_ID", 0x01, 0x0, 0x04, 0x0,
      32, 0, true, 4);
  RegisterInfoents[1] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_USER", 0x01, 0x4, 0x04, 0x0,
      32, 0, true, 5);
  RegisterInfoents[2] = mtca4u::RegisterInfoMap::RegisterInfo("MODULE_ID", 0x01, 0x0, 0x04, 0x1,
      32, 0, true, 6, "APP0");
  RegisterInfoents[3] = mtca4u::RegisterInfoMap::RegisterInfo("WORD_USER", 0x03, 0x4, 0x0C, 0x1,
      18, 3, false, 7, "APP0");

  mtca4u::RegisterInfoMap::const_iterator mapIter;
  std::vector<mtca4u::RegisterInfoMap::RegisterInfo>::const_iterator elementsIter;
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
      mtca4u::MapFileParser::splitStringAtLastDot(simple);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second == simple );

  stringPair = mtca4u::MapFileParser::splitStringAtLastDot(normal);
  BOOST_CHECK( stringPair.first == "MODULE" );
  BOOST_CHECK( stringPair.second == "REGISTER" );

  stringPair = mtca4u::MapFileParser::splitStringAtLastDot(withDots);
  BOOST_CHECK( stringPair.first == "MODULE.NAME.WITH.DOTS" );
  BOOST_CHECK( stringPair.second == "REGISTER" );

  stringPair = mtca4u::MapFileParser::splitStringAtLastDot(stillRegister);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second == "STILL_REGISTER" );

  stringPair = mtca4u::MapFileParser::splitStringAtLastDot(emptyRegister);
  BOOST_CHECK( stringPair.first == "MODULE" );
  BOOST_CHECK( stringPair.second.empty() );

  stringPair = mtca4u::MapFileParser::splitStringAtLastDot(justDot);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second.empty() );

}

void MapFileParserTest::testBadMappFileParse(){
  mtca4u::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("badMapFile.map"),
      mtca4u::MapFileException);
  try{
    fileparser.parse("badMapFile.map");
  } catch (mtca4u::MapFileException& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
        mtca4u::LibMapException::EX_MAP_FILE_PARSE_ERROR);
    BOOST_CHECK( std::string(mapFileException.what()) == "Error in mapp file: Empty register name in line 4!" );
  }
}
