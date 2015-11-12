#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "DeviceInfoMap.h"
#include "MapException.h"
#include "helperFunctions.h"

class DMapFileTest {
public:
	void testInsertElement();
	void testGetDeviceInfo();
	void testCheckForDuplicateElements();
	void testgetDeviceFileAndMapFileName();
	void testerrorElemErrTypeCoutStreamOperator();
	void testdRegisterInfoCoutStreamOperator();
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
		test_case* testdRegisterInfoCoutStreamOperator = BOOST_CLASS_TEST_CASE(
				&DMapFileTest::testdRegisterInfoCoutStreamOperator, DmapFileTestPtr);
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
		add(testdRegisterInfoCoutStreamOperator);
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
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent2;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent3;

	populateDummydRegisterInfoent(dRegisterInfoent1, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent2, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent3, dMapFileName);

	RegisterInfoMap.insert(dRegisterInfoent1);
	RegisterInfoMap.insert(dRegisterInfoent2);
	RegisterInfoMap.insert(dRegisterInfoent3);

	mtca4u::DeviceInfoMap::DeviceInfo* ptrList[3];
	ptrList[0] = &dRegisterInfoent1;
	ptrList[1] = &dRegisterInfoent2;
	ptrList[2] = &dRegisterInfoent3;
	int index;

	mtca4u::DeviceInfoMap::iterator it;
	for (it = RegisterInfoMap.begin(), index = 0;
			(it != RegisterInfoMap.end())  && (index < 3);
			++it, ++index) {
		BOOST_CHECK((compareDRegisterInfoents(*ptrList[index], *it)) == true);
	}
	BOOST_CHECK(RegisterInfoMap.getdmapFileSize() == 3);
}

void DMapFileTest::testGetDeviceInfo() {
	std::string dMapFileName = "dummy.map";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent2;

	populateDummydRegisterInfoent(dRegisterInfoent1, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent2, dMapFileName);

	RegisterInfoMap.insert(dRegisterInfoent1);
	RegisterInfoMap.insert(dRegisterInfoent2);

	mtca4u::DeviceInfoMap::DeviceInfo retrievedElement1;
	mtca4u::DeviceInfoMap::DeviceInfo retrievedElement2;
	mtca4u::DeviceInfoMap::DeviceInfo retrievedElement3;

	RegisterInfoMap.getDeviceInfo(dRegisterInfoent1.dev_name, retrievedElement1);
	RegisterInfoMap.getDeviceInfo(dRegisterInfoent2.dev_name, retrievedElement2);

	BOOST_CHECK((compareDRegisterInfoents(retrievedElement1, dRegisterInfoent1)) == true);
	BOOST_CHECK((compareDRegisterInfoents(retrievedElement2, dRegisterInfoent2)) == true);
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

void DMapFileTest::testCheckForDuplicateElements() {
	std::string dMapFileName = "dummy.map";
	std::string commonCardName = "common_card";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent2;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent3;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent4;

	populateDummydRegisterInfoent(dRegisterInfoent1, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent2, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent3, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent4, dMapFileName);

	dRegisterInfoent1.dev_name = commonCardName;
	dRegisterInfoent2.dev_name = commonCardName;
	dRegisterInfoent3.dev_name = commonCardName;

	mtca4u::DeviceInfoMap::ErrorList elementDuplications;
	RegisterInfoMap.insert(dRegisterInfoent1);
	BOOST_CHECK(RegisterInfoMap.check(elementDuplications,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR) ==
					true);

	RegisterInfoMap.insert(dRegisterInfoent2);
	RegisterInfoMap.insert(dRegisterInfoent3);
	RegisterInfoMap.insert(dRegisterInfoent4);

	RegisterInfoMap.check(elementDuplications,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR);

	int numberOfIncorrectLinesInFile = elementDuplications.errors.size();
	BOOST_CHECK(numberOfIncorrectLinesInFile == 2);

	std::list<mtca4u::DeviceInfoMap::ErrorList::ErrorElem>::iterator errorIterator;
	for (errorIterator = elementDuplications.errors.begin();
			errorIterator != elementDuplications.errors.end(); ++errorIterator) {
		bool doesDetectedElementsHaveSameName =
				(errorIterator->err_dev_1.dev_name ==
						errorIterator->err_dev_2.dev_name);
		BOOST_CHECK(doesDetectedElementsHaveSameName);
	}
}

void DMapFileTest::testgetDeviceFileAndMapFileName() {

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	dRegisterInfoent1.dev_file = "/dev/test";
	dRegisterInfoent1.map_file_name = "test_mapfile";

	std::pair<std::string, std::string> expected_pair("/dev/test",
			"test_mapfile");
	std::pair<std::string, std::string> actual_pair =
			dRegisterInfoent1.getDeviceFileAndMapFileName();
	BOOST_CHECK(expected_pair == actual_pair);
}

void DMapFileTest::testerrorElemErrTypeCoutStreamOperator() {
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

void DMapFileTest::testdRegisterInfoCoutStreamOperator() {
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	dRegisterInfoent1.dev_file = "/dev/dev1";
	dRegisterInfoent1.dev_name = "card1";
	dRegisterInfoent1.dmap_file_line_nr = 1;
	dRegisterInfoent1.dmap_file_name = "dummy.dmap";
	dRegisterInfoent1.map_file_name = "mapped_file";

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
	actual_file_stream << dRegisterInfoent1;

	BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFileTest::testdmapCoutStreamOperator() {
	std::string dMapFileName = "dummy.dmap";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	populateDummydRegisterInfoent(dRegisterInfoent1, dMapFileName, "card1", "/dev/dev1",
			"map_file");
	RegisterInfoMap.insert(dRegisterInfoent1);

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
	actual_file_stream << RegisterInfoMap;
	BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFileTest::testerrorElemCoutStreamOperator() {
	std::string dMapFileName = "dummy.map";
	//std::string commonCardName = "common_card";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent2;

	populateDummydRegisterInfoent(dRegisterInfoent1, "dummy.dmap", "card1", "/dev/dev1",
			"map_file");
	populateDummydRegisterInfoent(dRegisterInfoent2, "dummy.dmap", "card1", "/dev/dev1",
			"map_file");

	dRegisterInfoent1.dmap_file_line_nr = 1;
	dRegisterInfoent2.dmap_file_line_nr = 2;

	mtca4u::DeviceInfoMap::ErrorList::ErrorElem error_element(
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
			mtca4u::DeviceInfoMap::ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME,
			dRegisterInfoent1, dRegisterInfoent2);
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

void DMapFileTest::testErrorListCoutStreamOperator() {
	std::string dMapFileName = "dummy.dmap";
	std::string commonCardName = "card1";
	mtca4u::DeviceInfoMap RegisterInfoMap(dMapFileName);

	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent1;
	mtca4u::DeviceInfoMap::DeviceInfo dRegisterInfoent2;

	populateDummydRegisterInfoent(dRegisterInfoent1, dMapFileName);
	populateDummydRegisterInfoent(dRegisterInfoent2, dMapFileName);

	dRegisterInfoent1.dev_name = commonCardName;
	dRegisterInfoent2.dev_name = commonCardName;

	dRegisterInfoent1.dmap_file_line_nr = 1;
	dRegisterInfoent2.dmap_file_line_nr = 2;

	mtca4u::DeviceInfoMap::ErrorList elementDuplications;
	RegisterInfoMap.insert(dRegisterInfoent1);
	RegisterInfoMap.insert(dRegisterInfoent2);

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
