#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "MapFileParser.h"
#include "ExceptionMap.h"
#include "helperFunctions.h"
#include <sstream>

class MapFileParserTest{
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
  mtca4u::mapFileParser fileparser;
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
  mtca4u::mapFileParser map_file_parser;
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
  mtca4u::mapFileParser map_file_parser;
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
  mtca4u::mapFileParser map_file_parser;
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
  mtca4u::mapFileParser map_file_parser1;
  mtca4u::mapFileParser map_file_parser2;
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
  mtca4u::mapFileParser map_file_parser;
  boost::shared_ptr<mtca4u::mapFile> ptrmapFile =
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
  mtca4u::mapFile::mapElem mapElement1("WORD_FIRMWARE", 0x00000001,
                                       0x00000000, 0x00000004,
                                       0x0, 32, 0, true, 5);
  mtca4u::mapFile::mapElem mapElement2("WORD_COMPILATION", 0x00000001,
                                       0x00000004, 0x00000004,
                                       0x00000000, 32, 0, true, 6);
  mtca4u::mapFile::mapElem mapElement3("WORD_STATUS", 0x00000001,
                                       0x00000008, 0x00000004,
                                       0x00000000, 32, 0, true, 7);
  mtca4u::mapFile::mapElem mapElement4("WORD_USER1", 0x00000001,
                                       0x0000000C, 0x00000004,
                                       0x00000000, 32, 0, true, 8);
  mtca4u::mapFile::mapElem mapElement5("WORD_USER2", 0x00000001,
                                       0x00000010, 0x00000004,
                                       0x00000000, 32, 0, 0, 9);

  mtca4u::mapFile::mapElem* ptrList[5];
  ptrList[0] = &mapElement1;
  ptrList[1] = &mapElement2;
  ptrList[2] = &mapElement3;
  ptrList[3] = &mapElement4;
  ptrList[4] = &mapElement5;

  int index;
  mtca4u::mapFile::iterator it;
  for( it = ptrmapFile->begin(), index = 0;
      it != ptrmapFile->end(); ++it, ++index){
    BOOST_CHECK((compareMapElements(*ptrList[index], *it)) == true);
  }
}

void MapFileParserTest::testGoodMappFileParse () {
  mtca4u::mapFileParser map_file_parser;
  boost::shared_ptr<mtca4u::mapFile> ptrmapFile =
      map_file_parser.parse("goodMapFile.map");

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrieve, retrievedValue);
  BOOST_CHECK(retrievedValue == "2.5");

  std::vector< mtca4u::mapFile::mapElem > mapElements(11);

  mapElements[0] = mtca4u::mapFile::mapElem("WORD_FIRMWARE", 0x01, 0x0, 0x04, 0x0,
					    32, 0, true, 5, "BOARD");
  mapElements[1] = mtca4u::mapFile::mapElem("WORD_COMPILATION", 0x01, 0x04, 0x04, 0x0,
					    32, 0, true, 6, "BOARD");
  mapElements[2] = mtca4u::mapFile::mapElem("WORD_STATUS", 0x01, 0x08, 0x04, 0x01,
					    32, 0, true, 7, "APP0");
  mapElements[3] = mtca4u::mapFile::mapElem("WORD_SCRATCH", 0x01, 0x08, 0x04, 0x01,
					    16, 0, true, 8, "APP0");
  mapElements[4] = mtca4u::mapFile::mapElem("MODULE0", 0x02, 0x10, 0x08, 0x01,
					    32, 0, true, 9, "APP0");
  mapElements[5] = mtca4u::mapFile::mapElem("MODULE1", 0x02, 0x20, 0x08, 0x01,
					    32, 0, true, 10, "APP0");
  mapElements[6] = mtca4u::mapFile::mapElem("WORD_USER1", 0x01, 0x10, 0x04, 0x01,
					    16, 3, true, 14, "MODULE0"); 
  mapElements[7] = mtca4u::mapFile::mapElem("WORD_USER2", 0x01, 0x14, 0x04, 0x01,
					    18, 5, false, 15, "MODULE0");
  mapElements[8] = mtca4u::mapFile::mapElem("WORD_USER1", 0x01, 0x20, 0x04, 0x01,
					    16, 3, true, 16, "MODULE1");
  mapElements[9] = mtca4u::mapFile::mapElem("WORD_USER2", 0x01, 0x24, 0x04, 0x01,
					    18, 5, false, 17, "MODULE1");
  mapElements[10] = mtca4u::mapFile::mapElem("REGISTER", 0x01, 0x00, 0x04, 0x02,
					    32, 0, true, 20, "MODULE.NAME.WITH.DOTS");

  mtca4u::mapFile::const_iterator mapIter;
    std::vector<mtca4u::mapFile::mapElem>::const_iterator elementsIter;
 for( mapIter = ptrmapFile->begin(), elementsIter = mapElements.begin();
      mapIter != ptrmapFile->end() && elementsIter != mapElements.end();
      ++mapIter, ++elementsIter){
     std::stringstream message;
     message << "Failed comparison on Register '" << (*elementsIter).reg_name
	   << "', module '" << (elementsIter->reg_module) << "'";
     BOOST_CHECK_MESSAGE( compareMapElements(*mapIter, *elementsIter) == true,
			  message.str());
  }

}

void MapFileParserTest::testMixedMapFileParse () {
    mtca4u::mapFileParser map_file_parser;
    boost::shared_ptr<mtca4u::mapFile> ptrmapFile =
        map_file_parser.parse("mixedMapFile.map");
    
    std::vector< mtca4u::mapFile::mapElem > mapElements(4);

  mapElements[0] = mtca4u::mapFile::mapElem("WORD_FIRMWARE_ID", 0x01, 0x0, 0x04, 0x0,
					    32, 0, true, 4);
  mapElements[1] = mtca4u::mapFile::mapElem("WORD_USER", 0x01, 0x4, 0x04, 0x0,
					    32, 0, true, 5);
  mapElements[2] = mtca4u::mapFile::mapElem("MODULE_ID", 0x01, 0x0, 0x04, 0x1,
					    32, 0, true, 6, "APP0");
  mapElements[3] = mtca4u::mapFile::mapElem("WORD_USER", 0x03, 0x4, 0x0C, 0x1,
					    18, 3, false, 7, "APP0");

  mtca4u::mapFile::const_iterator mapIter;
  std::vector<mtca4u::mapFile::mapElem>::const_iterator elementsIter;
  for( mapIter = ptrmapFile->begin(), elementsIter = mapElements.begin();
       mapIter != ptrmapFile->end() && elementsIter != mapElements.end();
       ++mapIter, ++elementsIter){
     std::stringstream message;
     message << "Failed comparison on Register '" << (*elementsIter).reg_name
	   << "', module '" << (elementsIter->reg_module) << "'";
     BOOST_CHECK_MESSAGE( compareMapElements(*mapIter, *elementsIter) == true,
			  message.str());
  }

}

void MapFileParserTest::testSplitStringAtLastDot(){
  std::string simple("SIMPLE_REGISTER");
  std::string normal("MODULE.REGISTER");
  std::string withDots("MODULE.NAME.WITH.DOTS.REGISTER");
  std::string stillRegister(".STILL_REGISTER");
  std::string emptyRegister("MODULE.");
  std::string justDot(".");

  std::pair<std::string, std::string> stringPair =  
      mtca4u::mapFileParser::splitStringAtLastDot(simple);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second == simple );

  stringPair = mtca4u::mapFileParser::splitStringAtLastDot(normal);
  BOOST_CHECK( stringPair.first == "MODULE" );
  BOOST_CHECK( stringPair.second == "REGISTER" );
  
  stringPair = mtca4u::mapFileParser::splitStringAtLastDot(withDots);
  BOOST_CHECK( stringPair.first == "MODULE.NAME.WITH.DOTS" );
  BOOST_CHECK( stringPair.second == "REGISTER" );
  
  stringPair = mtca4u::mapFileParser::splitStringAtLastDot(stillRegister);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second == "STILL_REGISTER" );
  
  stringPair = mtca4u::mapFileParser::splitStringAtLastDot(emptyRegister);
  BOOST_CHECK( stringPair.first == "MODULE" );
  BOOST_CHECK( stringPair.second.empty() );

  stringPair = mtca4u::mapFileParser::splitStringAtLastDot(justDot);
  BOOST_CHECK( stringPair.first.empty() );
  BOOST_CHECK( stringPair.second.empty() );
  
}

void MapFileParserTest::testBadMappFileParse(){
  mtca4u::mapFileParser fileparser;
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
