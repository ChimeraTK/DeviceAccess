#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "helperFunctions.h"
#include "ExceptionMap.h"

class MapFileTest{
public:
  void testInsertElement();
  void testInsertMetadata();
  void testGetRegisterInfo();
  void testGetMetaData();
  void testCheckRegistersOfSameName();
  void testCheckRegisterAddressOverlap();
  void testMetadataCoutStreamOperator();
  void testMapElemCoutStreamOperator();
  void testErrElemTypeCoutStreamOperator();
  void testErrorElemCoutStreamOperator();
  void testErrorListCoutStreamOperator();
  void testMapFileCoutStreamOperator();
  void testGetRegistersInModule();
  static void testmapElem();
};




class mapFileTestSuite : public test_suite{
public:
  mapFileTestSuite(): test_suite("mapFile class test suite"){
    boost::shared_ptr<MapFileTest>
    MapFileTestPtr(new MapFileTest);

    test_case* testInsertElement =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testInsertElement,
	                      MapFileTestPtr);
    test_case* teestInsertMetadata =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testInsertMetadata,
	                      MapFileTestPtr);
    test_case* testGetRegisterInfo =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testGetRegisterInfo,
	                      MapFileTestPtr);
    test_case* testGetMetaData =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testGetMetaData,
	                      MapFileTestPtr);
    test_case* testCheckRegistersOfSameName =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testCheckRegistersOfSameName,
	                      MapFileTestPtr);
    test_case* testCheckRegisterAddressOverlap =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testCheckRegisterAddressOverlap,
	                      MapFileTestPtr);
    test_case* testMetadataCoutStreamOperator =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testMetadataCoutStreamOperator,
	                      MapFileTestPtr);
    test_case* testMapElemCoutStreamOperator =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testMapElemCoutStreamOperator,
	                      MapFileTestPtr);
    test_case* testErrElemTypeCoutStreamOperator =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testErrElemTypeCoutStreamOperator,
	                      MapFileTestPtr);
    test_case* testErrorElemCoutStreamOperator =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testErrorElemCoutStreamOperator,
	                      MapFileTestPtr);
    test_case* testErrorListCoutStreamOperator =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testErrorListCoutStreamOperator,
	                      MapFileTestPtr);
    test_case* testMapFileCoutStreamOperator =
	BOOST_CLASS_TEST_CASE(&MapFileTest::testMapFileCoutStreamOperator,
	                      MapFileTestPtr);
    add(testInsertElement);
    add(teestInsertMetadata);
    add(testGetRegisterInfo);
    add(testGetMetaData);
    add(testCheckRegistersOfSameName);
    add(testCheckRegisterAddressOverlap);
    add(testMetadataCoutStreamOperator);
    add(testMapElemCoutStreamOperator);
    add(testErrElemTypeCoutStreamOperator);
    add(testErrorElemCoutStreamOperator);
    add(testErrorListCoutStreamOperator);
    add(testMapFileCoutStreamOperator);

    add(BOOST_TEST_CASE(MapFileTest::testmapElem));
    add(BOOST_CLASS_TEST_CASE(&MapFileTest::testGetRegistersInModule, MapFileTestPtr));
  }
};

test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value =
      "mapFile class test suite";
  framework::master_test_suite().add(new mapFileTestSuite());

  return NULL;
}

void MapFileTest::testInsertElement(){
  mtca4u::mapFile dummyMapFile("dummy.map");
  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 2);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 1);
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_3", 4);
  mtca4u::mapFile::mapElem mapElementModule1("COMMON_REGISTER_NAME", 2, 8, 8, 1, 32, 0, true, 1, "Module1");
  mtca4u::mapFile::mapElem mapElementModule2("COMMON_REGISTER_NAME", 2, 16, 8, 1, 32, 0, true, 2, "Module2");

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);
  dummyMapFile.insert(mapElementModule1);
  dummyMapFile.insert(mapElementModule2);

  mtca4u::mapFile const & constDummyMapFile = dummyMapFile;

  mtca4u::mapFile::mapElem* ptrList[5];
  ptrList[0] = &mapElement1;
  ptrList[1] = &mapElement2;
  ptrList[2] = &mapElement3;
  ptrList[3] = &mapElementModule1;
  ptrList[4] = &mapElementModule2;
  
  int index;
  mtca4u::mapFile::iterator it;
  mtca4u::mapFile::const_iterator const_it;
  for( it = dummyMapFile.begin(), index = 0;
       (it != dummyMapFile.end()) && (index < 3);
       ++it, ++index){
    BOOST_CHECK((compareMapElements(*ptrList[index], *it)) == true);
  }
  for( const_it = constDummyMapFile.begin(), index = 0;
       (const_it != constDummyMapFile.end()) && (index < 3);
       ++const_it, ++index){
    BOOST_CHECK((compareMapElements(*ptrList[index], *const_it)) == true);
  }
  BOOST_CHECK(dummyMapFile.getMapFileSize() == 5);
}

void MapFileTest::testInsertMetadata(){
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::metaData metaData1("HW_VERSION", "1.6");
  mtca4u::mapFile::metaData metaData2("FW_VERSION", "2.5");
  mtca4u::mapFile::metaData metaData3("TEST", "Some additional information");

  dummyMapFile.insert(metaData1);
  dummyMapFile.insert(metaData2);
  dummyMapFile.insert(metaData3);

  std::string metaDataNameToRetrive;
  std::string retrivedValue;

  metaDataNameToRetrive = "HW_VERSION";
  dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue);
  BOOST_CHECK(retrivedValue == "1.6");

  metaDataNameToRetrive = "FW_VERSION";
  dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue);
  BOOST_CHECK(retrivedValue == "2.5");

  metaDataNameToRetrive = "TEST";
  dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue);
  BOOST_CHECK(retrivedValue == "Some additional information");
}

void MapFileTest::testGetRegisterInfo () {
  mtca4u::mapFile dummyMapFile("dummy.map");
  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 2, 0, 8);
  mtca4u::mapFile::mapElem mapElementModule1("COMMON_REGISTER_NAME", 2, 8, 8, 0, 32, 0, true, 1, "Module1");
  mtca4u::mapFile::mapElem mapElementModule2("COMMON_REGISTER_NAME", 2, 16, 8, 0, 32, 0, true, 2, "Module2");
  mtca4u::mapFile::mapElem reterivedMapElement;

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElementModule1);
  dummyMapFile.insert(mapElementModule2);

  dummyMapFile.getRegisterInfo("TEST_REGISTER_NAME_1", reterivedMapElement);
  BOOST_CHECK(compareMapElements(mapElement1, reterivedMapElement) == true);

  dummyMapFile.getRegisterInfo("COMMON_REGISTER_NAME", reterivedMapElement, "Module1");
  BOOST_CHECK(compareMapElements(mapElementModule1, reterivedMapElement) == true);

  dummyMapFile.getRegisterInfo("COMMON_REGISTER_NAME", reterivedMapElement, "Module2");
  BOOST_CHECK(compareMapElements(mapElementModule2, reterivedMapElement) == true);

  BOOST_CHECK_THROW(
      dummyMapFile.getRegisterInfo("some_name", reterivedMapElement),
      mtca4u::MapFileException);
  try{
    dummyMapFile.getRegisterInfo("some_name", reterivedMapElement);
  } catch(mtca4u::MapFileException& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
  }

  dummyMapFile.getRegisterInfo(0, reterivedMapElement);
  BOOST_CHECK(compareMapElements(mapElement1, reterivedMapElement) == true);
  BOOST_CHECK_THROW(
      dummyMapFile.getRegisterInfo(3, reterivedMapElement),
      mtca4u::MapFileException);
  try{
    dummyMapFile.getRegisterInfo(3, reterivedMapElement);
  } catch(mtca4u::MapFileException& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
  }

}

void MapFileTest::testGetMetaData () {
  mtca4u::mapFile dummyMapFile("dummy.map");
  mtca4u::mapFile::metaData metaData1("HW_VERSION", "1.6");
  dummyMapFile.insert(metaData1);

  std::string metaDataNameToRetrive;
  std::string retrivedValue;

  metaDataNameToRetrive = "HW_VERSION";
  dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue);
  BOOST_CHECK(retrivedValue == "1.6");

  metaDataNameToRetrive = "some_name";
  BOOST_CHECK_THROW(
      dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue),
      mtca4u::MapFileException);
  try{
    dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue);
  } catch (mtca4u::MapFileException& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::LibMapException::EX_NO_METADATA_IN_MAP_FILE);
  }
}

void MapFileTest::testCheckRegistersOfSameName(){
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_1", 1, 4, 4, 1);
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_1", 1, 8, 4, 0);
  mtca4u::mapFile::mapElem mapElement4("TEST_REGISTER_NAME_2", 1, 8, 4, 2);
  mtca4u::mapFile::mapElem mapElementModule1("COMMON_REGISTER_NAME", 2, 8, 8, 3, 32, 0, true, 1, "Module1");
  mtca4u::mapFile::mapElem mapElementModule2("COMMON_REGISTER_NAME", 2, 16, 8, 3, 32, 0, true, 2, "Module2");

  mtca4u::mapFile::errorList errorList;
  dummyMapFile.insert(mapElement1);
  // check after the first element to cover the specific branch (special case)
  dummyMapFile.check(errorList, mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 0);

  dummyMapFile.insert(mapElementModule1);
  dummyMapFile.insert(mapElementModule2);
  dummyMapFile.check(errorList, mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 0);

  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);
  dummyMapFile.insert(mapElement4);
  dummyMapFile.check(errorList, mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 2);

  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator;
  for(errorIterator = errorList.errors.begin();
      errorIterator != errorList.errors.end();
      ++errorIterator){

      BOOST_CHECK(errorIterator->err_type ==
		  mtca4u::mapFile::errorList::errorElem::NONUNIQUE_REGISTER_NAME);
      BOOST_CHECK(errorIterator->type ==
		  mtca4u::mapFile::errorList::errorElem::ERROR);
  }

  // duplicate identical entries is an error
  dummyMapFile.insert(mapElement4);
  // only get the errors. There also is an overlap warning now.
  dummyMapFile.check(errorList, mtca4u::mapFile::errorList::errorElem::ERROR);
  BOOST_CHECK(errorList.errors.size() == 3); 
}

void MapFileTest::testCheckRegisterAddressOverlap(){
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 1, 11, 4, 0);
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_3", 1, 10, 4, 0);
 // 4 overlaps with 1, but is not next to it in the list
  mtca4u::mapFile::mapElem mapElement4("TEST_REGISTER_NAME_4", 1, 3, 4, 0);
  mtca4u::mapFile::mapElem mapElement5("THE_WHOLE_MODULE", 2, 16, 8, 0);
  mtca4u::mapFile::mapElem mapElement6("REGISTER_1", 1, 16, 4, 0, 32, 0, true, 0, "THE_MODULE" );
  mtca4u::mapFile::mapElem mapElement7("REGISTER_2", 1, 20, 4, 0, 32, 0, true, 0, "THE_MODULE" );

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);
  dummyMapFile.insert(mapElement4);
  dummyMapFile.insert(mapElement5);
  dummyMapFile.insert(mapElement6);
  dummyMapFile.insert(mapElement7);

  mtca4u::mapFile::errorList errorList;
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::ERROR);
  BOOST_CHECK(errorList.errors.size() == 0);
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 2);
  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator;

  errorIterator = errorList.errors.begin();
  BOOST_CHECK(errorIterator->err_reg_1.reg_name == "TEST_REGISTER_NAME_3");
  BOOST_CHECK(errorIterator->err_reg_2.reg_name == "TEST_REGISTER_NAME_2");
  BOOST_CHECK(errorIterator->err_type ==
      mtca4u::mapFile::errorList::errorElem::WRONG_REGISTER_ADDRESSES);
  BOOST_CHECK(errorIterator->type ==
      mtca4u::mapFile::errorList::errorElem::WARNING);

  errorIterator++;
  BOOST_CHECK(errorIterator->err_reg_1.reg_name == "TEST_REGISTER_NAME_4");
  BOOST_CHECK(errorIterator->err_reg_2.reg_name == "TEST_REGISTER_NAME_1");
  BOOST_CHECK(errorIterator->err_type ==
      mtca4u::mapFile::errorList::errorElem::WRONG_REGISTER_ADDRESSES);
  BOOST_CHECK(errorIterator->type ==
      mtca4u::mapFile::errorList::errorElem::WARNING);
}

void MapFileTest::testMetadataCoutStreamOperator(){
  mtca4u::mapFile::metaData meta_data("metadata_name", "metadata_value");
  std::stringstream expected_stream;
  expected_stream << "METADATA-> NAME: \"" <<
      "metadata_name" << "\" VALUE: " << "metadata_value" << std::endl;

  std::stringstream actual_stream;
  actual_stream << meta_data;

  BOOST_CHECK(expected_stream.str() == actual_stream.str());
}

void MapFileTest::testMapElemCoutStreamOperator(){
  mtca4u::mapFile::mapElem mapElement1("Some_Register");
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 2, 4, 8, 1, 
				       18, 3, false, 0, "SomeModule");

  std::stringstream expected_stream;
  expected_stream << "Some_Register" << " 0x" 
		  << std::hex << 0 << " 0x" << 0 << " 0x" << 0 << " 0x"<< 0 << std::dec
		  << " 32 0 true";
  expected_stream << "TEST_REGISTER_NAME_2" << " 0x" 
		  << std::hex << 2 << " 0x" << 4 << " 0x" << 8 << " 0x" << 1 << std::dec
		  << " 18 3 false SomeModule";
  std::stringstream actual_stream;
  actual_stream << mapElement1;
  actual_stream << mapElement2;

  BOOST_CHECK(expected_stream.str() == actual_stream.str());
}

void MapFileTest::testErrElemTypeCoutStreamOperator(){
  std::stringstream stream1;
  stream1 << mtca4u::mapFile::errorList::errorElem::ERROR;
  BOOST_CHECK(stream1.str() == "ERROR");

  std::stringstream stream2;
  stream2 << mtca4u::mapFile::errorList::errorElem::WARNING;
  BOOST_CHECK(stream2.str() == "WARNING");

  std::stringstream stream3;
  stream3 << mtca4u::mapFile::errorList::errorElem::TYPE(4);
  BOOST_CHECK(stream3.str() == "UNKNOWN");
}

void MapFileTest::testErrorElemCoutStreamOperator(){
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 1, 3, 4, 0);

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);

  mtca4u::mapFile::errorList errorList;
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::WARNING);
  std::stringstream expected_stream;
  expected_stream << mtca4u::mapFile::errorList::errorElem::WARNING <<
      ": Found two registers with overlapping addresses: \"" <<
      "TEST_REGISTER_NAME_2" << "\" and \"" << "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" << " in lines "
      << 0 << " and " << 0;
  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator;
  errorIterator = errorList.errors.begin();

  std::stringstream actual_stream;
  actual_stream << *errorIterator;
  BOOST_CHECK(expected_stream.str() == actual_stream.str());

  mtca4u::mapFile dummyMapFile1("dummy.map");
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement4("TEST_REGISTER_NAME_1", 1, 4, 4, 1);
  dummyMapFile1.insert(mapElement3);
  dummyMapFile1.insert(mapElement4);
  mtca4u::mapFile::errorList errorList1;

  dummyMapFile1.check(errorList1,
                      mtca4u::mapFile::errorList::errorElem::WARNING);
  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator1;
  errorIterator1 = errorList1.errors.begin();
  std::stringstream actual_stream1;
  actual_stream1 << *errorIterator1;

  std::stringstream expected_stream1;
  expected_stream1 << mtca4u::mapFile::errorList::errorElem::ERROR <<
      ": Found two registers with the same name: \"" <<
      "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" <<\
      " in lines " << 0 << " and " << 0;

  BOOST_CHECK(expected_stream1.str() == actual_stream1.str());
}

inline void
MapFileTest::testErrorListCoutStreamOperator () {
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 1, 4, 4, 0);
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_1", 1, 10, 4, 0);
  mtca4u::mapFile::mapElem mapElement4("TEST_REGISTER_NAME_3", 1, 12, 4, 0);

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);
  dummyMapFile.insert(mapElement4);

  mtca4u::mapFile::errorList errorList;
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::WARNING);

  std::stringstream expected_stream;
  expected_stream << mtca4u::mapFile::errorList::errorElem::ERROR <<
      ": Found two registers with the same name: \"" <<
      "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" <<\
      " in lines " << 0 << " and " << 0 <<std::endl;
  expected_stream << mtca4u::mapFile::errorList::errorElem::WARNING <<
      ": Found two registers with overlapping addresses: \"" <<
      "TEST_REGISTER_NAME_3" << "\" and \"" << "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" << " in lines "
      << 0 << " and " << 0 << std::endl;

  std::stringstream actual_stream;
  actual_stream << errorList;
  BOOST_CHECK(expected_stream.str() == actual_stream.str());

}

void MapFileTest::testMapFileCoutStreamOperator(){
  mtca4u::mapFile dummyMapFile("dummy.map");
  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1");
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 2, 4, 8, 1, 
				       18, 3, false, 0, "TEST_MODULE");
  mtca4u::mapFile::metaData metaData1("HW_VERSION", "1.6");

  dummyMapFile.insert(metaData1);
  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);

  std::stringstream expected_stream;
  expected_stream << "=======================================" << std::endl;
  expected_stream << "MAP FILE NAME: " << "dummy.map" << std::endl;
  expected_stream << "---------------------------------------" << std::endl;
  expected_stream << "METADATA-> NAME: \"" <<
      "HW_VERSION" << "\" VALUE: " << "1.6" << std::endl;
  expected_stream << "---------------------------------------" << std::endl;
  expected_stream << "TEST_REGISTER_NAME_1" << " 0x" 
		  << std::hex << 0 << " 0x" << 0 << " 0x" << 0 << " 0x" << 0 << std::dec
		  << " 32 0 true" << std::endl;
  expected_stream << "TEST_REGISTER_NAME_2" << " 0x" 
		  << std::hex << 2 << " 0x" << 4 << " 0x" << 8 << " 0x" << 1 << std::dec
		  << " 18 3 false TEST_MODULE" << std::endl;
  expected_stream << "=======================================";

  std::stringstream actual_stream;
  actual_stream << dummyMapFile;

  BOOST_CHECK(actual_stream.str() == expected_stream.str());
}

void MapFileTest::testmapElem(){
  // just test the constructor. Default and all arguments
  mtca4u::mapFile::mapElem defaultMapElem;
  BOOST_CHECK( defaultMapElem.reg_name.empty() );
  BOOST_CHECK( defaultMapElem.reg_elem_nr == 0 );
  BOOST_CHECK( defaultMapElem.reg_address == 0 );
  BOOST_CHECK( defaultMapElem.reg_size == 0 );
  BOOST_CHECK( defaultMapElem.reg_bar == 0 );
  BOOST_CHECK( defaultMapElem.reg_width == 32 );
  BOOST_CHECK( defaultMapElem.reg_frac_bits == 0 );
  BOOST_CHECK( defaultMapElem.reg_signed == true );
  BOOST_CHECK( defaultMapElem.line_nr == 0 );
  BOOST_CHECK( defaultMapElem.reg_module.empty() );

  // Set values which are all different from the default
  mtca4u::mapFile::mapElem myMapElem( "MY_NAME",
				      4, // nElements
				      0x42, //address
				      16, // size
				      3, // bar
				      18, // width
				      5, // frac_bits
				      false, // signed
				      123, //line_nr
				      "MY_MODULE");
  BOOST_CHECK( myMapElem.reg_name == "MY_NAME" );
  BOOST_CHECK( myMapElem.reg_elem_nr == 4 );
  BOOST_CHECK( myMapElem.reg_address == 0x42 );
  BOOST_CHECK( myMapElem.reg_size == 16 );
  BOOST_CHECK( myMapElem.reg_bar == 3 );
  BOOST_CHECK( myMapElem.reg_width == 18 );
  BOOST_CHECK( myMapElem.reg_frac_bits == 5 );
  BOOST_CHECK( myMapElem.reg_signed == false );
  BOOST_CHECK( myMapElem.line_nr == 123 );
  BOOST_CHECK( myMapElem.reg_module == "MY_MODULE" );
}

void MapFileTest::testGetRegistersInModule(){
  mtca4u::mapFile someMapFile("some.map");
  mtca4u::mapFile::mapElem module0_register1("REGISTER_1", 1, 0x0, 4, 0, 32, 0, true, 0, "MODULE_BAR0");
  mtca4u::mapFile::mapElem module1_register1("REGISTER_1", 1, 0x0, 4, 1, 32, 0, true, 0, "MODULE_BAR1");
  mtca4u::mapFile::mapElem module0_aregister2("A_REGISTER_2", 1, 0x4, 4, 0, 32, 0, true, 0, "MODULE_BAR0");
  mtca4u::mapFile::mapElem module1_aregister2("A_REGISTER_2", 1, 0x4, 4, 1, 32, 0, true, 0, "MODULE_BAR1");
  mtca4u::mapFile::mapElem module0_register3("REGISTER_3", 1, 0x8, 4, 0, 32, 0, true, 0, "MODULE_BAR0");
  mtca4u::mapFile::mapElem module1_register3("REGISTER_3", 1, 0x8, 4, 1, 32, 0, true, 0, "MODULE_BAR1");
  mtca4u::mapFile::mapElem module0_register4("REGISTER_4", 1, 0xC, 4, 0, 32, 0, true, 0, "MODULE_BAR0");
  mtca4u::mapFile::mapElem module1_register4("REGISTER_4", 1, 0xC, 4, 1, 32, 0, true, 0, "MODULE_BAR1");
  
  // add stuff from two different modules, interleaved. We need all registers back in 
  // alphabetical order.
  someMapFile.insert( module0_register1 );
  someMapFile.insert( module1_register1 );
  someMapFile.insert( module0_aregister2 );
  someMapFile.insert( module1_aregister2 );
  someMapFile.insert( module0_register3 );
  someMapFile.insert( module1_register3 );
  someMapFile.insert( module0_register4 );
  someMapFile.insert( module1_register4 );

  std::list< mtca4u::mapFile::mapElem > resultList = someMapFile.getRegistersInModule( "MODULE_BAR1" );
  BOOST_CHECK( resultList.size() == 4 );
  
  // create a reference list to iterate
  std::list< mtca4u::mapFile::mapElem > referenceList;
  referenceList.push_back(  module1_aregister2 );
  referenceList.push_back(  module1_register1 );
  referenceList.push_back(  module1_register3 );
  referenceList.push_back(  module1_register4 );

  std::list< mtca4u::mapFile::mapElem >::const_iterator resultIter;
  std::list< mtca4u::mapFile::mapElem >::const_iterator referenceIter;
  for (resultIter = resultList.begin(), referenceIter = referenceList.begin();
       (resultIter != resultList.end()) && (referenceIter != referenceList.end());
       ++resultIter, ++referenceIter){
    std::stringstream message;
    message << "Failed comparison on Register '" << referenceIter->reg_name
	   << "', module '" << referenceIter->reg_module << "'";
    BOOST_CHECK( compareMapElements( *resultIter, *referenceIter ) == true );
  }

  std::list< mtca4u::mapFile::mapElem > shouldBeEmptyList = someMapFile.getRegistersInModule( "MODULE_BAR5" );
  BOOST_CHECK( shouldBeEmptyList.empty() );
}
