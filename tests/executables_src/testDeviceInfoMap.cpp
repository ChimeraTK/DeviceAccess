// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

///@todo FIXME My dynamic init header is a hack. Change the test to use
/// BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
#include "DeviceInfoMap.h"
#include "Exception.h"
#include "helperFunctions.h"

using namespace boost::unit_test_framework;
namespace ChimeraTK {
  using namespace ChimeraTK;
}

class DeviceInfoMapTest {
 public:
  void testInsertElement();
  void testGetDeviceInfo();
  void testCheckForDuplicateElements();
  void testGetDeviceFileAndMapFileName();
  void testErrorElemErrTypeStreamOperator();
  void testDeviceInfoStreamOperator();
  void testDeviceInfoMapStreamOperator();
  void testErrorElemStreamOperator();
  void testErrorListStreamOperator();
};

class DeviceInfoMapTestSuite : public test_suite {
 public:
  DeviceInfoMapTestSuite() : test_suite("DeviceInfoMap test suite") {
    boost::shared_ptr<DeviceInfoMapTest> deviceInfoMapTest(new DeviceInfoMapTest());

    test_case* insertElementsTestCase = BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testInsertElement, deviceInfoMapTest);
    test_case* getDeviceInfoTestCase = BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testGetDeviceInfo, deviceInfoMapTest);
    test_case* checkForDuplicateElementEntries =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testCheckForDuplicateElements, deviceInfoMapTest);
    test_case* testGetDeviceFileAndMapFileName =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testGetDeviceFileAndMapFileName, deviceInfoMapTest);
    test_case* testErrorElemErrTypeStreamOperator =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testErrorElemErrTypeStreamOperator, deviceInfoMapTest);
    test_case* testDeviceInfoStreamOperator =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testDeviceInfoStreamOperator, deviceInfoMapTest);
    test_case* testDeviceInfoMapStreamOperator =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testDeviceInfoMapStreamOperator, deviceInfoMapTest);
    test_case* testErrorElemStreamOperator =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testErrorElemStreamOperator, deviceInfoMapTest);

    test_case* testErrorListStreamOperator =
        BOOST_CLASS_TEST_CASE(&DeviceInfoMapTest::testErrorListStreamOperator, deviceInfoMapTest);
    add(insertElementsTestCase);
    add(getDeviceInfoTestCase);
    add(checkForDuplicateElementEntries);
    add(testGetDeviceFileAndMapFileName);
    add(testErrorElemErrTypeStreamOperator);
    add(testDeviceInfoStreamOperator);
    add(testDeviceInfoMapStreamOperator);
    add(testErrorElemStreamOperator);
    add(testErrorListStreamOperator);
  }
};

bool init_unit_test() {
  framework::master_test_suite().p_name.value = "DeviceInfoMap test suite";
  framework::master_test_suite().add(new DeviceInfoMapTestSuite());

  return true;
  ;
}

void DeviceInfoMapTest::testInsertElement() {
  std::string dMapFileName = "dummy.map";
  ChimeraTK::DeviceInfoMap deviceInfoMap(dMapFileName);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo3;

  populateDummyDeviceInfo(deviceInfo1, dMapFileName);
  populateDummyDeviceInfo(deviceInfo2, dMapFileName);
  populateDummyDeviceInfo(deviceInfo3, dMapFileName);

  deviceInfoMap.insert(deviceInfo1);
  deviceInfoMap.insert(deviceInfo2);
  deviceInfoMap.insert(deviceInfo3);

  ChimeraTK::DeviceInfoMap::DeviceInfo* ptrList[3];
  ptrList[0] = &deviceInfo1;
  ptrList[1] = &deviceInfo2;
  ptrList[2] = &deviceInfo3;
  int index;

  ChimeraTK::DeviceInfoMap::iterator it;
  for(it = deviceInfoMap.begin(), index = 0; (it != deviceInfoMap.end()) && (index < 3); ++it, ++index) {
    BOOST_CHECK((compareDeviceInfos(*ptrList[index], *it)) == true);
  }
  BOOST_CHECK(deviceInfoMap.getSize() == 3);
}

void DeviceInfoMapTest::testGetDeviceInfo() {
  std::string dMapFileName = "dummy.map";
  ChimeraTK::DeviceInfoMap RegisterInfoMap(dMapFileName);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;

  populateDummyDeviceInfo(deviceInfo1, dMapFileName);
  populateDummyDeviceInfo(deviceInfo2, dMapFileName);

  RegisterInfoMap.insert(deviceInfo1);
  RegisterInfoMap.insert(deviceInfo2);

  ChimeraTK::DeviceInfoMap::DeviceInfo retrievedElement1;
  ChimeraTK::DeviceInfoMap::DeviceInfo retrievedElement2;
  ChimeraTK::DeviceInfoMap::DeviceInfo retrievedElement3;

  RegisterInfoMap.getDeviceInfo(deviceInfo1.deviceName, retrievedElement1);
  RegisterInfoMap.getDeviceInfo(deviceInfo2.deviceName, retrievedElement2);

  BOOST_CHECK((compareDeviceInfos(retrievedElement1, deviceInfo1)) == true);
  BOOST_CHECK((compareDeviceInfos(retrievedElement2, deviceInfo2)) == true);
  BOOST_CHECK_THROW(RegisterInfoMap.getDeviceInfo("invalid_card_name", retrievedElement3), ChimeraTK::logic_error);
}

void DeviceInfoMapTest::testCheckForDuplicateElements() {
  std::string dMapFileName = "dummy.map";
  std::string commonCardName = "common_card";
  ChimeraTK::DeviceInfoMap RegisterInfoMap(dMapFileName);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo3;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo4;

  populateDummyDeviceInfo(deviceInfo1, dMapFileName);
  populateDummyDeviceInfo(deviceInfo2, dMapFileName);
  populateDummyDeviceInfo(deviceInfo3, dMapFileName);
  populateDummyDeviceInfo(deviceInfo4, dMapFileName);

  deviceInfo1.deviceName = commonCardName;
  deviceInfo2.deviceName = commonCardName;
  deviceInfo3.deviceName = commonCardName;

  ChimeraTK::DeviceInfoMap::ErrorList elementDuplications;
  RegisterInfoMap.insert(deviceInfo1);
  BOOST_CHECK(
      RegisterInfoMap.check(elementDuplications, ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR) == true);

  RegisterInfoMap.insert(deviceInfo2);
  RegisterInfoMap.insert(deviceInfo3);
  RegisterInfoMap.insert(deviceInfo4);

  RegisterInfoMap.check(elementDuplications, ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR);

  int numberOfIncorrectLinesInFile = elementDuplications._errors.size();
  BOOST_CHECK(numberOfIncorrectLinesInFile == 2);

  std::list<ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem>::iterator errorIterator;
  for(errorIterator = elementDuplications._errors.begin(); errorIterator != elementDuplications._errors.end();
      ++errorIterator) {
    bool doesDetectedElementsHaveSameName =
        (errorIterator->_errorDevice1.deviceName == errorIterator->_errorDevice2.deviceName);
    BOOST_CHECK(doesDetectedElementsHaveSameName);
  }
}

void DeviceInfoMapTest::testGetDeviceFileAndMapFileName() {
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  deviceInfo1.uri = "/dev/test";
  deviceInfo1.mapFileName = "test_mapfile";

  std::pair<std::string, std::string> expected_pair("/dev/test", "test_mapfile");
  std::pair<std::string, std::string> actual_pair = deviceInfo1.getDeviceFileAndMapFileName();
  BOOST_CHECK(expected_pair == actual_pair);
}

void DeviceInfoMapTest::testErrorElemErrTypeStreamOperator() {
  std::stringstream file_stream1;
  file_stream1 << ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR;
  BOOST_CHECK(file_stream1.str() == "ERROR");

  std::stringstream file_stream2;
  file_stream2 << ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::WARNING;
  BOOST_CHECK(file_stream2.str() == "WARNING");
}

void DeviceInfoMapTest::testDeviceInfoStreamOperator() {
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  deviceInfo1.uri = "/dev/dev1";
  deviceInfo1.deviceName = "card1";
  deviceInfo1.dmapFileLineNumber = 1;
  deviceInfo1.dmapFileName = "dummy.dmap";
  deviceInfo1.mapFileName = "mapped_file";

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
  actual_file_stream << deviceInfo1;

  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DeviceInfoMapTest::testDeviceInfoMapStreamOperator() {
  std::string dMapFileName = "dummy.dmap";
  ChimeraTK::DeviceInfoMap deviceInfoMap(dMapFileName);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  populateDummyDeviceInfo(deviceInfo1, dMapFileName, "card1", "/dev/dev1", "map_file");
  deviceInfoMap.insert(deviceInfo1);

  std::stringstream expected_file_stream;
  expected_file_stream << "=======================================" << std::endl;
  expected_file_stream << "MAP FILE NAME: "
                       << "dummy.dmap" << std::endl;
  expected_file_stream << "---------------------------------------" << std::endl;

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
  actual_file_stream << deviceInfoMap;
  BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DeviceInfoMapTest::testErrorElemStreamOperator() {
  std::string dMapFileName = "dummy.map";
  // std::string commonCardName = "common_card";
  ChimeraTK::DeviceInfoMap RegisterInfoMap(dMapFileName);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;

  populateDummyDeviceInfo(deviceInfo1, "dummy.dmap", "card1", "/dev/dev1", "map_file");
  populateDummyDeviceInfo(deviceInfo2, "dummy.dmap", "card1", "/dev/dev1", "map_file");

  deviceInfo1.dmapFileLineNumber = 1;
  deviceInfo2.dmapFileLineNumber = 2;

  ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem error_element(ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
      ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME, deviceInfo1, deviceInfo2);
  std::stringstream expected_file_stream;
  expected_file_stream << ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR
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

void DeviceInfoMapTest::testErrorListStreamOperator() {
  std::string dMapFileName = "dummy.dmap";
  std::string commonCardName = "card1";
  ChimeraTK::DeviceInfoMap RegisterInfoMap(dMapFileName);

  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo1;
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceInfo2;

  populateDummyDeviceInfo(deviceInfo1, dMapFileName);
  populateDummyDeviceInfo(deviceInfo2, dMapFileName);

  deviceInfo1.deviceName = commonCardName;
  deviceInfo2.deviceName = commonCardName;

  deviceInfo1.dmapFileLineNumber = 1;
  deviceInfo2.dmapFileLineNumber = 2;

  ChimeraTK::DeviceInfoMap::ErrorList elementDuplications;
  RegisterInfoMap.insert(deviceInfo1);
  RegisterInfoMap.insert(deviceInfo2);

  RegisterInfoMap.check(elementDuplications, ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR);
  std::stringstream expected_file_stream;
  expected_file_stream << ChimeraTK::DeviceInfoMap::ErrorList::ErrorElem::ERROR
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
