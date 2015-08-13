#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "DMapFile.h"
#include "MapException.h"
#include "helperFunctions.h"

class DMapFileTest {
 public:
  void testInsertElement();
  void testGetDeviceInfo();
  void testCheckForDuplicateElements();
  void testgetDeviceFileAndMapFileName();
  void testerrorElemErrTypeCoutStreamOperator();
  void testdmapElemCoutStreamOperator();
  void testdmapCoutStreamOperator();
  void testerrorElemCoutStreamOperator();
  void testErrorListCoutStreamOperator();
};

class DMapFileTestSuite : public test_suite {
 public:
  DMapFileTestSuite() : test_suite("dMapFile class test suite") {

    boost::shared_ptr<DMapFileTest> DmapFileTestPtr(new DMapFileTest());

    test_case* insertElementsTestCase = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testInsertElement, DmapFileTestPtr);
    test_case* getDeviceInfoTestCase = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testGetDeviceInfo, DmapFileTestPtr);
    test_case* checkForDuplicateElementEntries = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testCheckForDuplicateElements, DmapFileTestPtr);
    test_case* testgetDeviceFileAndMapFileName = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testgetDeviceFileAndMapFileName, DmapFileTestPtr);
    test_case* testerrorElemErrTypeCoutStreamOperator = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testerrorElemErrTypeCoutStreamOperator, DmapFileTestPtr);
    test_case* testdmapElemCoutStreamOperator = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testdmapElemCoutStreamOperator, DmapFileTestPtr);
    test_case* testdmapCoutStreamOperator = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testdmapCoutStreamOperator, DmapFileTestPtr);
    test_case* testerrorElemCoutStreamOperator = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testerrorElemCoutStreamOperator, DmapFileTestPtr);

    test_case* testErrorListCoutStreamOperator = BOOST_CLASS_TEST_CASE(
        &DMapFileTest::testErrorListCoutStreamOperator, DmapFileTestPtr);
    add(insertElementsTestCase);
    add(getDeviceInfoTestCase);
    add(checkForDuplicateElementEntries);
    add(testgetDeviceFileAndMapFileName);
    add(testerrorElemErrTypeCoutStreamOperator);
    add(testdmapElemCoutStreamOperator);
    add(testdmapCoutStreamOperator);
    add(testerrorElemCoutStreamOperator);
    add(testErrorListCoutStreamOperator);
  }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "dmapFile test suite";
  framework::master_test_suite().add(new DMapFileTestSuite());

  return NULL;
}

void DMapFileTest::testInsertElement() {
  std::string dMapFileName = "dummy.map";
  mtca4u::DMapFile mapFile(dMapFileName);

  mtca4u::DMapFile::dmapElem dMapElement1;
  mtca4u::DMapFile::dmapElem dMapElement2;
  mtca4u::DMapFile::dmapElem dMapElement3;

  populateDummydMapElement(dMapElement1, dMapFileName);
  populateDummydMapElement(dMapElement2, dMapFileName);
  populateDummydMapElement(dMapElement3, dMapFileName);

  mapFile.insert(dMapElement1);
  mapFile.insert(dMapElement2);
  mapFile.insert(dMapElement3);

  mtca4u::DMapFile::dmapElem* ptrList[3];
  ptrList[0] = &dMapElement1;
  ptrList[1] = &dMapElement2;
  ptrList[2] = &dMapElement3;
  int index;

  mtca4u::DMapFile::iterator it;
  for (it = mapFile.begin(), index = 0; 
       (it != mapFile.end())  && (index < 3);
       ++it, ++index) {
    BOOST_CHECK((compareDMapElements(*ptrList[index], *it)) == true);
  }
  BOOST_CHECK(mapFile.getdmapFileSize() == 3);
}

void DMapFileTest::testGetDeviceInfo() {
  std::string dMapFileName = "dummy.map";
  mtca4u::DMapFile mapFile(dMapFileName);

  mtca4u::DMapFile::dmapElem dMapElement1;
  mtca4u::DMapFile::dmapElem dMapElement2;

  populateDummydMapElement(dMapElement1, dMapFileName);
  populateDummydMapElement(dMapElement2, dMapFileName);

  mapFile.insert(dMapElement1);
  mapFile.insert(dMapElement2);

  mtca4u::DMapFile::dmapElem retrievedElement1;
  mtca4u::DMapFile::dmapElem retrievedElement2;
  mtca4u::DMapFile::dmapElem retrievedElement3;

  mapFile.getDeviceInfo(dMapElement1.dev_name, retrievedElement1);
  mapFile.getDeviceInfo(dMapElement2.dev_name, retrievedElement2);

  BOOST_CHECK((compareDMapElements(retrievedElement1, dMapElement1)) == true);
  BOOST_CHECK((compareDMapElements(retrievedElement2, dMapElement2)) == true);
  BOOST_CHECK_THROW(
      mapFile.getDeviceInfo("invalid_card_name", retrievedElement3),
      mtca4u::DMapFileException);
  try {
    mapFile.getDeviceInfo("invalid_card_name", retrievedElement3);
  }
  catch (mtca4u::DMapFileException& dMapException) {
    BOOST_CHECK(dMapException.getID() ==
                mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
  }
}

void DMapFileTest::testCheckForDuplicateElements() {
  std::string dMapFileName = "dummy.map";
  std::string commonCardName = "common_card";
  mtca4u::DMapFile mapFile(dMapFileName);

  mtca4u::DMapFile::dmapElem dMapElement1;
  mtca4u::DMapFile::dmapElem dMapElement2;
  mtca4u::DMapFile::dmapElem dMapElement3;
  mtca4u::DMapFile::dmapElem dMapElement4;

  populateDummydMapElement(dMapElement1, dMapFileName);
  populateDummydMapElement(dMapElement2, dMapFileName);
  populateDummydMapElement(dMapElement3, dMapFileName);
  populateDummydMapElement(dMapElement4, dMapFileName);

  dMapElement1.dev_name = commonCardName;
  dMapElement2.dev_name = commonCardName;
  dMapElement3.dev_name = commonCardName;

  mtca4u::DMapFile::errorList elementDuplications;
  mapFile.insert(dMapElement1);
  BOOST_CHECK(mapFile.check(elementDuplications,
                            mtca4u::DMapFile::errorList::errorElem::ERROR) ==
              true);

  mapFile.insert(dMapElement2);
  mapFile.insert(dMapElement3);
  mapFile.insert(dMapElement4);

  mapFile.check(elementDuplications,
                mtca4u::DMapFile::errorList::errorElem::ERROR);

  int numberOfIncorrectLinesInFile = elementDuplications.errors.size();
  BOOST_CHECK(numberOfIncorrectLinesInFile == 2);

  std::list<mtca4u::DMapFile::errorList::errorElem>::iterator errorIterator;
  for (errorIterator = elementDuplications.errors.begin();
       errorIterator != elementDuplications.errors.end(); ++errorIterator) {
    bool doesDetectedElementsHaveSameName =
        (errorIterator->err_dev_1.dev_name ==
         errorIterator->err_dev_2.dev_name);
    BOOST_CHECK(doesDetectedElementsHaveSameName);
  }
}

void DMapFileTest::testgetDeviceFileAndMapFileName() {

  mtca4u::DMapFile::dmapElem dMapElement1;
  dMapElement1.dev_file = "/dev/test";
  dMapElement1.map_file_name = "test_mapfile";

  std::pair<std::string, std::string> expected_pair("/dev/test",
                                                    "test_mapfile");
  std::pair<std::string, std::string> actual_pair =
      dMapElement1.getDeviceFileAndMapFileName();
  BOOST_CHECK(expected_pair == actual_pair);
}

void DMapFileTest::testerrorElemErrTypeCoutStreamOperator() {
  std::stringstream file_stream1;
  file_stream1 << mtca4u::DMapFile::errorList::errorElem::ERROR;
  BOOST_CHECK(file_stream1.str() == "ERROR");

  std::stringstream file_stream2;
  file_stream2 << mtca4u::DMapFile::errorList::errorElem::WARNING;
  BOOST_CHECK(file_stream2.str() == "WARNING");

  std::stringstream file_stream3;
  file_stream3 << (mtca4u::DMapFile::errorList::errorElem::TYPE)4;
  BOOST_CHECK(file_stream3.str() == "UNKNOWN");
}

void DMapFileTest::testdmapElemCoutStreamOperator() {
  mtca4u::DMapFile::dmapElem dMapElement1;
  dMapElement1.dev_file = "/dev/dev1";
  dMapElement1.dev_name = "card1";
  dMapElement1.dmap_file_line_nr = 1;
  dMapElement1.dmap_file_name = "dummy.dmap";
  dMapElement1.map_file_name = "mapped_file";

  std::stringstream expected_file_stream;
  expected_file_stream << "("
                       << "dummy.dmap"
                       << ") NAME: "
                       << "card1"
                       << " DEV : "
                       << "/dev/dev1"
                       << " MAP : "
                       << "mapped_file";

  std::stringstream actual_file_stream;
  actual_file_stream << dMapElement1;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFileTest::testdmapCoutStreamOperator() {
  std::string dMapFileName = "dummy.dmap";
  mtca4u::DMapFile mapFile(dMapFileName);

  mtca4u::DMapFile::dmapElem dMapElement1;
  populateDummydMapElement(dMapElement1, dMapFileName, "card1", "/dev/dev1",
                           "map_file");
  mapFile.insert(dMapElement1);

  std::stringstream expected_file_stream;
  expected_file_stream << "======================================="
                       << std::endl;
  expected_file_stream << "MAP FILE NAME: "
                       << "dummy.dmap" << std::endl;
  expected_file_stream << "---------------------------------------"
                       << std::endl;

  expected_file_stream << "("
                       << "dummy.dmap"
                       << ") NAME: "
                       << "card1"
                       << " DEV : "
                       << "/dev/dev1"
                       << " MAP : "
                       << "map_file" << std::endl;

  expected_file_stream << "=======================================";

  std::stringstream actual_file_stream;
  actual_file_stream << mapFile;
  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFileTest::testerrorElemCoutStreamOperator() {
  std::string dMapFileName = "dummy.map";
  std::string commonCardName = "common_card";
  mtca4u::DMapFile mapFile(dMapFileName);

  mtca4u::DMapFile::dmapElem dMapElement1;
  mtca4u::DMapFile::dmapElem dMapElement2;

  populateDummydMapElement(dMapElement1, "dummy.dmap", "card1", "/dev/dev1",
                           "map_file");
  populateDummydMapElement(dMapElement2, "dummy.dmap", "card1", "/dev/dev1",
                           "map_file");

  dMapElement1.dmap_file_line_nr = 1;
  dMapElement2.dmap_file_line_nr = 2;

  mtca4u::DMapFile::errorList::errorElem error_element(
      mtca4u::DMapFile::errorList::errorElem::ERROR,
      mtca4u::DMapFile::errorList::errorElem::NONUNIQUE_DEVICE_NAME,
      dMapElement1, dMapElement2);
  std::stringstream expected_file_stream;
  expected_file_stream
      << mtca4u::DMapFile::errorList::errorElem::ERROR
      << ": Found two devices with the same name but different properties: \""
      << "card1"
      << "\" in file \""
      << "dummy.dmap"
      << "\" in line " << 1 << " and \""
      << "dummy.dmap"
      << "\" in line " << 2;

  std::stringstream actual_file_stream;
  actual_file_stream << error_element;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFileTest::testErrorListCoutStreamOperator() {
  std::string dMapFileName = "dummy.dmap";
  std::string commonCardName = "card1";
  mtca4u::DMapFile mapFile(dMapFileName);

  mtca4u::DMapFile::dmapElem dMapElement1;
  mtca4u::DMapFile::dmapElem dMapElement2;

  populateDummydMapElement(dMapElement1, dMapFileName);
  populateDummydMapElement(dMapElement2, dMapFileName);

  dMapElement1.dev_name = commonCardName;
  dMapElement2.dev_name = commonCardName;

  dMapElement1.dmap_file_line_nr = 1;
  dMapElement2.dmap_file_line_nr = 2;

  mtca4u::DMapFile::errorList elementDuplications;
  mapFile.insert(dMapElement1);
  mapFile.insert(dMapElement2);

  mapFile.check(elementDuplications,
                mtca4u::DMapFile::errorList::errorElem::ERROR);
  std::stringstream expected_file_stream;
  expected_file_stream
      << mtca4u::DMapFile::errorList::errorElem::ERROR
      << ": Found two devices with the same name but different properties: \""
      << "card1"
      << "\" in file \""
      << "dummy.dmap"
      << "\" in line " << 1 << " and \""
      << "dummy.dmap"
      << "\" in line " << 2 << std::endl;

  std::stringstream actual_file_stream;
  actual_file_stream << elementDuplications;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}
