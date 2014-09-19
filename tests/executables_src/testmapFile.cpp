#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "helperFunctions.h"
#include "exlibmap.h"

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

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);

  mtca4u::mapFile const & constDummyMapFile = dummyMapFile;

  mtca4u::mapFile::mapElem* ptrList[3];
  ptrList[0] = &mapElement1;
  ptrList[1] = &mapElement2;
  ptrList[2] = &mapElement3;
  int index;

  mtca4u::mapFile::iterator it;
  mtca4u::mapFile::const_iterator const_it;
  for( it = dummyMapFile.begin(), index = 0;
      it != dummyMapFile.end(); ++it, ++index){
    BOOST_CHECK((compareMapElements(*ptrList[index], *it)) == true);
  }
  for( const_it = constDummyMapFile.begin(), index = 0;
      const_it != constDummyMapFile.end(); ++const_it, ++index){
    BOOST_CHECK((compareMapElements(*ptrList[index], *const_it)) == true);
  }
  BOOST_CHECK(dummyMapFile.getMapFileSize() == 3);
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
  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 2);
  mtca4u::mapFile::mapElem reterivedMapElement;

  dummyMapFile.insert(mapElement1);

  dummyMapFile.getRegisterInfo("TEST_REGISTER_NAME_1", reterivedMapElement);
  BOOST_CHECK(compareMapElements(mapElement1, reterivedMapElement) == true);

  BOOST_CHECK_THROW(
      dummyMapFile.getRegisterInfo("some_name", reterivedMapElement),
      mtca4u::exMapFile);
  try{
    dummyMapFile.getRegisterInfo("some_name", reterivedMapElement);
  } catch(mtca4u::exMapFile& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_NO_REGISTER_IN_MAP_FILE);
  }

  dummyMapFile.getRegisterInfo(0, reterivedMapElement);
  BOOST_CHECK(compareMapElements(mapElement1, reterivedMapElement) == true);
  BOOST_CHECK_THROW(
      dummyMapFile.getRegisterInfo(1, reterivedMapElement),
      mtca4u::exMapFile);
  try{
    dummyMapFile.getRegisterInfo(1, reterivedMapElement);
  } catch(mtca4u::exMapFile& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_NO_REGISTER_IN_MAP_FILE);
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
      mtca4u::exMapFile);
  try{
    dummyMapFile.getMetaData(metaDataNameToRetrive, retrivedValue);
  } catch (mtca4u::exMapFile& mapFileException){
    BOOST_CHECK(mapFileException.getID() ==
	mtca4u::exLibMap::EX_NO_METADATA_IN_MAP_FILE);
  }
}

void MapFileTest::testCheckRegistersOfSameName(){
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_1", 1, 4, 4, 1);
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_1", 1, 8, 4, 0);
  mtca4u::mapFile::mapElem mapElement4("TEST_REGISTER_NAME_2", 1, 8, 4, 2);

  mtca4u::mapFile::errorList errorList;
  dummyMapFile.insert(mapElement1);
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 0);

  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);
  dummyMapFile.insert(mapElement4);
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 2);

  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator;
  for(errorIterator = errorList.errors.begin();\
  errorIterator != errorList.errors.end();\
  ++errorIterator){
    bool areNonUniqueRegistersPresent =
	((errorIterator->err_reg_1.reg_name ==
	    errorIterator->err_reg_2.reg_name) &&\
	    ((errorIterator->err_reg_1.reg_address != \
		errorIterator->err_reg_2.reg_address) ||\
		(errorIterator->err_reg_1.reg_bar != \
		    errorIterator->err_reg_2.reg_bar) ||\
		    (errorIterator->err_reg_1.reg_elem_nr != \
			errorIterator->err_reg_2.reg_elem_nr) ||\
			(errorIterator->err_reg_1.reg_size != \
			    errorIterator->err_reg_2.reg_size)));
    BOOST_CHECK(areNonUniqueRegistersPresent);
    BOOST_CHECK(errorIterator->err_type ==
	mtca4u::mapFile::errorList::errorElem::NONUNIQUE_REGISTER_NAME);
    BOOST_CHECK(errorIterator->type ==
	mtca4u::mapFile::errorList::errorElem::ERROR);
  }
}

void MapFileTest::testCheckRegisterAddressOverlap(){
  mtca4u::mapFile dummyMapFile("dummy.map");

  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1", 1, 0, 4, 0);
  mtca4u::mapFile::mapElem mapElement2("TEST_REGISTER_NAME_2", 1, 3, 4, 0);
  mtca4u::mapFile::mapElem mapElement3("TEST_REGISTER_NAME_3", 1, 11, 4, 0);
  mtca4u::mapFile::mapElem mapElement4("TEST_REGISTER_NAME_4", 1, 10, 4, 0);

  dummyMapFile.insert(mapElement1);
  dummyMapFile.insert(mapElement2);
  dummyMapFile.insert(mapElement3);
  dummyMapFile.insert(mapElement4);

  mtca4u::mapFile::errorList errorList;
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::ERROR);
  BOOST_CHECK(errorList.errors.size() == 0);
  dummyMapFile.check(errorList,
                     mtca4u::mapFile::errorList::errorElem::WARNING);
  BOOST_CHECK(errorList.errors.size() == 2);
  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator;

  errorIterator = errorList.errors.begin();
  BOOST_CHECK(errorIterator->err_reg_1.reg_name == "TEST_REGISTER_NAME_2");
  BOOST_CHECK(errorIterator->err_reg_2.reg_name == "TEST_REGISTER_NAME_1");
  BOOST_CHECK(errorIterator->err_type ==
      mtca4u::mapFile::errorList::errorElem::WRONG_REGISTER_ADDRESSES);
  BOOST_CHECK(errorIterator->type ==
      mtca4u::mapFile::errorList::errorElem::WARNING);

  errorIterator++;
  BOOST_CHECK(errorIterator->err_reg_1.reg_name == "TEST_REGISTER_NAME_4");
  BOOST_CHECK(errorIterator->err_reg_2.reg_name == "TEST_REGISTER_NAME_3");
  BOOST_CHECK(errorIterator->err_type ==
      mtca4u::mapFile::errorList::errorElem::WRONG_REGISTER_ADDRESSES);
  BOOST_CHECK(errorIterator->type ==
      mtca4u::mapFile::errorList::errorElem::WARNING);
}

void MapFileTest::testMetadataCoutStreamOperator(){
  mtca4u::mapFile::metaData meta_data("metadata_name", "metadata_value");
  std::stringstream expected_file_stream;
  expected_file_stream << "METADATA-> NAME: \"" <<
      "metadata_name" << "\" VALUE: " << "metadata_value" << std::endl;

  std::stringstream actual_file_stream;
  actual_file_stream << meta_data;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void MapFileTest::testMapElemCoutStreamOperator(){
  mtca4u::mapFile::mapElem mapElem("Some_Register");
  std::stringstream expected_file_stream;
  expected_file_stream << "Some_Register" << " 0x" <<
      std::hex << 0 << " 0x" << 0 << " 0x" << 0 << " 0x" <<
      0 << std::dec;
  std::stringstream actual_file_stream;
  actual_file_stream << mapElem;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void MapFileTest::testErrElemTypeCoutStreamOperator(){
  std::stringstream file_stream1;
  file_stream1 << mtca4u::mapFile::errorList::errorElem::ERROR;
  BOOST_CHECK(file_stream1.str() == "ERROR");

  std::stringstream file_stream2;
  file_stream2 << mtca4u::mapFile::errorList::errorElem::WARNING;
  BOOST_CHECK(file_stream2.str() == "WARNING");

  std::stringstream file_stream3;
  file_stream3 << mtca4u::mapFile::errorList::errorElem::TYPE(4);
  BOOST_CHECK(file_stream3.str() == "UNKNOWN");
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
  std::stringstream expected_file_stream;
  expected_file_stream << mtca4u::mapFile::errorList::errorElem::WARNING <<
      ": Found two registers with overlapping addresses: \"" <<
      "TEST_REGISTER_NAME_2" << "\" and \"" << "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" << " in lines "
      << 0 << " and " << 0;
  std::list<mtca4u::mapFile::errorList::errorElem>::iterator errorIterator;
  errorIterator = errorList.errors.begin();

  std::stringstream actual_file_stream;
  actual_file_stream << *errorIterator;
  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());

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
  std::stringstream actual_file_stream1;
  actual_file_stream1 << *errorIterator1;

  std::stringstream expected_file_stream1;
  expected_file_stream1 << mtca4u::mapFile::errorList::errorElem::ERROR <<
      ": Found two registers with the same name: \"" <<
      "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" <<\
      " in lines " << 0 << " and " << 0;

  BOOST_CHECK(expected_file_stream1.str() == actual_file_stream1.str());
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

  std::stringstream expected_file_stream;
  expected_file_stream << mtca4u::mapFile::errorList::errorElem::ERROR <<
      ": Found two registers with the same name: \"" <<
      "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" <<\
      " in lines " << 0 << " and " << 0 <<std::endl;
  expected_file_stream << mtca4u::mapFile::errorList::errorElem::WARNING <<
      ": Found two registers with overlapping addresses: \"" <<
      "TEST_REGISTER_NAME_3" << "\" and \"" << "TEST_REGISTER_NAME_1" <<
      "\" in file " << "dummy.map" << " in lines "
      << 0 << " and " << 0 << std::endl;

  std::stringstream actual_file_stream;
  actual_file_stream << errorList;
  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());

}

void MapFileTest::testMapFileCoutStreamOperator(){
  mtca4u::mapFile dummyMapFile("dummy.map");
  mtca4u::mapFile::mapElem mapElement1("TEST_REGISTER_NAME_1");
  mtca4u::mapFile::metaData metaData1("HW_VERSION", "1.6");

  dummyMapFile.insert(metaData1);
  dummyMapFile.insert(mapElement1);

  std::stringstream expected_stream;
  expected_stream << "=======================================" << std::endl;
  expected_stream << "MAP FILE NAME: " << "dummy.map" << std::endl;
  expected_stream << "---------------------------------------" << std::endl;
  expected_stream << "METADATA-> NAME: \"" <<
      "HW_VERSION" << "\" VALUE: " << "1.6" << std::endl;
  expected_stream << "---------------------------------------" << std::endl;
  expected_stream << "TEST_REGISTER_NAME_1" << " 0x" <<
      std::hex << 0 << " 0x" << 0 << " 0x" << 0 << " 0x" <<
      0 << std::dec << std::endl;
  expected_stream << "=======================================";

  std::stringstream actual_stream;
  actual_stream << dummyMapFile;
  BOOST_CHECK(actual_stream.str() == expected_stream.str());
}
