#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "mapFileParser.h"
#include "exlibmap.h"
#include "helperFunctions.h"

class MapFileParserTest{
public:
  void testFileDoesNotExist();
  void testInvalidMetadata();
  void testMandatoryRegisterFieldMissing();
  void testIncorrectRegisterWidth();
  void testFracBits();
  void testGoodMapFileParse();
};
class MapFileParserTestSuite : public test_suite{
public:
  MapFileParserTestSuite():test_suite("mapFileParser class test suite"){

    boost::shared_ptr<MapFileParserTest>
    mapFileParserTestPtr(new MapFileParserTest());

    test_case* testFileDoesNotExist =
	BOOST_CLASS_TEST_CASE(&MapFileParserTest::testFileDoesNotExist,
	                      mapFileParserTestPtr);
    test_case* testInvalidMetadata =
	BOOST_CLASS_TEST_CASE(&MapFileParserTest::testInvalidMetadata,
	                      mapFileParserTestPtr);
    test_case* testMandatoryRegisterFieldMissing =
	BOOST_CLASS_TEST_CASE(
	    &MapFileParserTest::testMandatoryRegisterFieldMissing,
	                      mapFileParserTestPtr);
    test_case* testIncorrectRegisterWidth =
	BOOST_CLASS_TEST_CASE(&MapFileParserTest::testIncorrectRegisterWidth,
	                      mapFileParserTestPtr);
    test_case* testFracBits =
	BOOST_CLASS_TEST_CASE(&MapFileParserTest::testFracBits,
		                      mapFileParserTestPtr);
    test_case* testGoodMapFile =
	BOOST_CLASS_TEST_CASE(&MapFileParserTest::testGoodMapFileParse,
		                      mapFileParserTestPtr);

    add(testFileDoesNotExist);
    add(testInvalidMetadata);
    add(testMandatoryRegisterFieldMissing);
    add(testIncorrectRegisterWidth);
    add(testGoodMapFile);
    add(testFracBits);

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
                    mtca4u::exMapFile);
  try{
    fileparser.parse("NonexistentFile.map");
  } catch (mtca4u::exMapFile& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_CANNOT_OPEN_MAP_FILE);
  }
}

void MapFileParserTest::testInvalidMetadata(){
  mtca4u::mapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("invalid_metadata.map"),
                    mtca4u::exMapFile);

  try{
    map_file_parser.parse("invalid_metadata.map");
  } catch (mtca4u::exMapFile &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_MAP_FILE_PARSE_ERROR);
  }
}

void MapFileParserTest::testMandatoryRegisterFieldMissing () {
  mtca4u::mapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("MandatoryRegisterfIeldMissing.map"),
                    mtca4u::exMapFile);
  try{
    map_file_parser.parse("MandatoryRegisterfIeldMissing.map");
  }
  catch (mtca4u::exMapFile &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_MAP_FILE_PARSE_ERROR);
  }
}

void MapFileParserTest::testIncorrectRegisterWidth () {
  mtca4u::mapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth.map"),
                    mtca4u::exMapFile);
  try{
    map_file_parser.parse("IncorrectRegisterWidth.map");
  }
  catch (mtca4u::exMapFile &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_MAP_FILE_PARSE_ERROR);
  }
}

void MapFileParserTest::testFracBits () {
  mtca4u::mapFileParser map_file_parser1;
  mtca4u::mapFileParser map_file_parser2;
  BOOST_CHECK_THROW(map_file_parser1.parse("IncorrectFracBits1.map"),
                    mtca4u::exMapFile);
  BOOST_CHECK_THROW(map_file_parser2.parse("IncorrectFracBits2.map"),
                    mtca4u::exMapFile);
  try{
    map_file_parser1.parse("IncorrectFracBits1.map");
  }
  catch (mtca4u::exMapFile &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_MAP_FILE_PARSE_ERROR);
  }

  try{
    map_file_parser1.parse("IncorrectFracBits2.map");
  }
  catch (mtca4u::exMapFile &mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_MAP_FILE_PARSE_ERROR);
  }

}

void MapFileParserTest::testGoodMapFileParse () {
  mtca4u::mapFileParser map_file_parser;
  boost::shared_ptr<mtca4u::mapFile> ptrmapFile =
      map_file_parser.parse("goodMapFile.map");

  std::string metaDataNameToRetrive;
  std::string retrivedValue;

  metaDataNameToRetrive = "HW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrive, retrivedValue);
  BOOST_CHECK(retrivedValue == "1.6");

  metaDataNameToRetrive = "FW_VERSION";
  ptrmapFile->getMetaData(metaDataNameToRetrive, retrivedValue);
  BOOST_CHECK(retrivedValue == "2.5");

  /* TODO: remove default assignments to unused fields in the parse
   * function and
   * move it all to the constructor */
  mtca4u::mapFile::mapElem mapElement1("WORD_FIRMWARE", 0x00000001,
                                       0x00000000, 0x00000004,
                                       0xFFFFFFFF, 32, 0, true, 5);
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

