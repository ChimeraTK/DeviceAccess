#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "DeviceInfoMap.h"
#include "MapException.h"
#include "helperFunctions.h"

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

		test_case* insertElementsTestCase = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testInsertElement, deviceInfoMapTest);
		test_case* getDeviceInfoTestCase = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testGetDeviceInfo, deviceInfoMapTest);
		test_case* checkForDuplicateElementEntries = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testCheckForDuplicateElements, deviceInfoMapTest);
		test_case* testGetDeviceFileAndMapFileName = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testGetDeviceFileAndMapFileName, deviceInfoMapTest);
		test_case* testErrorElemErrTypeStreamOperator = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testErrorElemErrTypeStreamOperator, deviceInfoMapTest);
		test_case* testDeviceInfoStreamOperator = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testDeviceInfoStreamOperator, deviceInfoMapTest);
		test_case* testDeviceInfoMapStreamOperator = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testDeviceInfoMapStreamOperator, deviceInfoMapTest);
		test_case* testErrorElemStreamOperator = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testErrorElemStreamOperator, deviceInfoMapTest);

		test_case* testErrorListStreamOperator = BOOST_CLASS_TEST_CASE(
				&DeviceInfoMapTest::testErrorListStreamOperator, deviceInfoMapTest);
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

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value = "DeviceInfoMap test suite";
	framework::master_test_suite().add(new DeviceInfoMapTestSuite());

	return NULL;
}

void DeviceInfoMapTest::testInsertElement() {
	std::string dMapFileName = "dummy.map";
	mtca4u::DeviceInfoMap deviceInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo3;

	populateDummyDeviceInfo(deviceInfo1, dMapFileName);
	populateDummyDeviceInfo(deviceInfo2, dMapFileName);
	populateDummyDeviceInfo(deviceInfo3, dMapFileName);

	deviceInfoMap.insert(deviceInfo1);
	deviceInfoMap.insert(deviceInfo2);
	deviceInfoMap.insert(deviceInfo3);

	mtca4u::DeviceInfoMap::DeviceInfo* ptrList[3];
	ptrList[0] = &deviceInfo1;
	ptrList[1] = &deviceInfo2;
	ptrList[2] = &deviceInfo3;
	int index;

	mtca4u::DeviceInfoMap::iterator it;
	for (it = deviceInfoMap.begin(), index = 0;
			(it != deviceInfoMap.end())  && (index < 3);
			++it, ++index) {
		BOOST_CHECK((compareDeviceInfos(*ptrList[index], *it)) == true);
	}
	BOOST_CHECK(deviceInfoMap.getSize() == 3);
}

void DeviceInfoMapTest::testGetDeviceInfo() {
	std::string dMapFileName = "dummy.map";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;

	populateDummyDeviceInfo(deviceInfo1, dMapFileName);
	populateDummyDeviceInfo(deviceInfo2, dMapFileName);

	RegisterInfoMap.insert(deviceInfo1);
	RegisterInfoMap.insert(deviceInfo2);

	mtca4u::DeviceInfoMap::DeviceInfo retrievedElement1;
	mtca4u::DeviceInfoMap::DeviceInfo retrievedElement2;
	mtca4u::DeviceInfoMap::DeviceInfo retrievedElement3;

	RegisterInfoMap.getDeviceInfo(deviceInfo1._deviceName, retrievedElement1);
	RegisterInfoMap.getDeviceInfo(deviceInfo2._deviceName, retrievedElement2);

	BOOST_CHECK((compareDeviceInfos(retrievedElement1, deviceInfo1)) == true);
	BOOST_CHECK((compareDeviceInfos(retrievedElement2, deviceInfo2)) == true);
	BOOST_CHECK_THROW(
			RegisterInfoMap.getDeviceInfo("invalid_card_name", retrievedElement3),
			mtca4u::DMapFileException);
	try {
		RegisterInfoMap.getDeviceInfo("invalid_card_name", retrievedElement3);
	}
	catch (mtca4u::DMapFileException& dMapException) {
		BOOST_CHECK(dMapException.getID() ==
				mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}
}

void DeviceInfoMapTest::testCheckForDuplicateElements() {
	std::string dMapFileName = "dummy.map";
	std::string commonCardName = "common_card";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo3;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo4;

	populateDummyDeviceInfo(deviceInfo1, dMapFileName);
	populateDummyDeviceInfo(deviceInfo2, dMapFileName);
	populateDummyDeviceInfo(deviceInfo3, dMapFileName);
	populateDummyDeviceInfo(deviceInfo4, dMapFileName);

	deviceInfo1._deviceName = commonCardName;
	deviceInfo2._deviceName = commonCardName;
	deviceInfo3._deviceName = commonCardName;

	mtca4u::DeviceInfoMap::ErrorList elementDuplications;
	RegisterInfoMap.insert(deviceInfo1);
	BOOST_CHECK(RegisterInfoMap.check(elementDuplications,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR) ==
					true);

	RegisterInfoMap.insert(deviceInfo2);
	RegisterInfoMap.insert(deviceInfo3);
	RegisterInfoMap.insert(deviceInfo4);

	RegisterInfoMap.check(elementDuplications,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR);

	int numberOfIncorrectLinesInFile = elementDuplications._errors.size();
	BOOST_CHECK(numberOfIncorrectLinesInFile == 2);

	std::list<mtca4u::DeviceInfoMap::ErrorList::ErrorElem>::iterator errorIterator;
	for (errorIterator = elementDuplications._errors.begin();
			errorIterator != elementDuplications._errors.end(); ++errorIterator) {
		bool doesDetectedElementsHaveSameName =
				(errorIterator->_errorDevice1._deviceName ==
						errorIterator->_errorDevice2._deviceName);
		BOOST_CHECK(doesDetectedElementsHaveSameName);
	}
}

void DeviceInfoMapTest::testGetDeviceFileAndMapFileName() {

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	deviceInfo1._deviceFile = "/dev/test";
	deviceInfo1._mapFileName = "test_mapfile";

	std::pair<std::string, std::string> expected_pair("/dev/test",
			"test_mapfile");
	std::pair<std::string, std::string> actual_pair =
			deviceInfo1.getDeviceFileAndMapFileName();
	BOOST_CHECK(expected_pair == actual_pair);
}

void DeviceInfoMapTest::testErrorElemErrTypeStreamOperator() {
	std::stringstream file_stream1;
	file_stream1 << mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR;
	BOOST_CHECK(file_stream1.str() == "ERROR");

	std::stringstream file_stream2;
	file_stream2 << mtca4u::DeviceInfoMap::ErrorList::ErrorElem::WARNING;
	BOOST_CHECK(file_stream2.str() == "WARNING");

	std::stringstream file_stream3;
	file_stream3 << (mtca4u::DeviceInfoMap::ErrorList::ErrorElem::TYPE)4;
	BOOST_CHECK(file_stream3.str() == "UNKNOWN");
}

void DeviceInfoMapTest::testDeviceInfoStreamOperator() {
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	deviceInfo1._deviceFile = "/dev/dev1";
	deviceInfo1._deviceName = "card1";
	deviceInfo1._dmapFileLineNumber = 1;
	deviceInfo1._dmapFileName = "dummy.dmap";
	deviceInfo1._mapFileName = "mapped_file";

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
	mtca4u::DeviceInfoMap deviceInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	populateDummyDeviceInfo(deviceInfo1, dMapFileName, "card1", "/dev/dev1",
			"map_file");
	deviceInfoMap.insert(deviceInfo1);

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
	actual_file_stream << deviceInfoMap;
	BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DeviceInfoMapTest::testErrorElemStreamOperator() {
	std::string dMapFileName = "dummy.map";
	//std::string commonCardName = "common_card";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;

	populateDummyDeviceInfo(deviceInfo1, "dummy.dmap", "card1", "/dev/dev1",
			"map_file");
	populateDummyDeviceInfo(deviceInfo2, "dummy.dmap", "card1", "/dev/dev1",
			"map_file");

	deviceInfo1._dmapFileLineNumber = 1;
	deviceInfo2._dmapFileLineNumber = 2;

	mtca4u::DeviceInfoMap::ErrorList::ErrorElem error_element(
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME,
			deviceInfo1, deviceInfo2);
	std::stringstream expected_file_stream;
	expected_file_stream
	<< mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR
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
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;

	populateDummyDeviceInfo(deviceInfo1, dMapFileName);
	populateDummyDeviceInfo(deviceInfo2, dMapFileName);

	deviceInfo1._deviceName = commonCardName;
	deviceInfo2._deviceName = commonCardName;

	deviceInfo1._dmapFileLineNumber = 1;
	deviceInfo2._dmapFileLineNumber = 2;

	mtca4u::DeviceInfoMap::ErrorList elementDuplications;
	RegisterInfoMap.insert(deviceInfo1);
	RegisterInfoMap.insert(deviceInfo2);

	RegisterInfoMap.check(elementDuplications,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR);
	std::stringstream expected_file_stream;
	expected_file_stream
	<< mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR
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
