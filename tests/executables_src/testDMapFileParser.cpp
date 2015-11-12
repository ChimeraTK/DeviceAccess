#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "DMapFileParser.h"
#include "MapException.h"
#include "helperFunctions.h"

class DMapFileParserTest {
public:
	void testFileNotFound();
	void testErrorInDmapFile();
	void testNoDataInDmapFile();
	void testParseFile();
};

class DMapFileParserTestSuite : public test_suite {
public:
	DMapFileParserTestSuite() : test_suite("DMapFileParser class test suite") {
		boost::shared_ptr<DMapFileParserTest> DMapFileParserTestPtr(
				new DMapFileParserTest);

		test_case* testFileNotFound = BOOST_CLASS_TEST_CASE(
				&DMapFileParserTest::testFileNotFound, DMapFileParserTestPtr);
		test_case* testErrorInDmapFile = BOOST_CLASS_TEST_CASE(
				&DMapFileParserTest::testErrorInDmapFile, DMapFileParserTestPtr);
		test_case* testNoDataInDmapFile = BOOST_CLASS_TEST_CASE(
				&DMapFileParserTest::testNoDataInDmapFile, DMapFileParserTestPtr);
		test_case* testParseFile = BOOST_CLASS_TEST_CASE(
				&DMapFileParserTest::testParseFile, DMapFileParserTestPtr);

		add(testFileNotFound);
		add(testErrorInDmapFile);
		add(testParseFile);
		add(testNoDataInDmapFile);
	}
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value =
			"DMapFileParser class test suite";
	framework::master_test_suite().add(new DMapFileParserTestSuite());

	return NULL;
}

void DMapFileParserTest::testFileNotFound() {
	std::string file_path = "../dummypath.dmap";
	mtca4u::DMapFileParser fileParser;

	BOOST_CHECK_THROW(fileParser.parse(file_path), mtca4u::LibMapException);
	try {
		fileParser.parse(file_path);
	}
	catch (mtca4u::LibMapException& dMapFileParserException) {
		BOOST_CHECK(dMapFileParserException.getID() ==
				mtca4u::LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
	}
}

void DMapFileParserTest::testErrorInDmapFile() {
	std::string incorrect_dmap_file = "invalid.dmap";
	mtca4u::DMapFileParser fileParser;

	BOOST_CHECK_THROW(fileParser.parse(incorrect_dmap_file), mtca4u::LibMapException);
	try {
		fileParser.parse(incorrect_dmap_file);
	}
	catch (mtca4u::LibMapException& dMapFileParserException) {
		std::cout << dMapFileParserException;
		BOOST_CHECK(dMapFileParserException.getID() ==
				mtca4u::LibMapException::EX_DMAP_FILE_PARSE_ERROR);
	}
}

void DMapFileParserTest::testNoDataInDmapFile() {
	std::string empty_dmap_file = "empty.dmap";
	mtca4u::DMapFileParser fileParser;

	BOOST_CHECK_THROW(fileParser.parse(empty_dmap_file), mtca4u::LibMapException);
	try {
		fileParser.parse(empty_dmap_file);
	}
	catch (mtca4u::LibMapException& dMapFileParserException) {
		std::cout << dMapFileParserException;
		BOOST_CHECK(dMapFileParserException.getID() ==
				mtca4u::LibMapException::EX_NO_DMAP_DATA);
	}
}

void DMapFileParserTest::testParseFile() {
	std::string file_path = "valid.dmap";
	mtca4u::DMapFileParser fileParser;
	boost::shared_ptr<mtca4u::DeviceInfoMap> mapFilePtr = fileParser.parse(file_path);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo3;

	populateDummyDeviceInfo(deviceInfo1, "valid.dmap", "card1", "/dev/dev1",
			"goodMapFile_withoutModules.map");
	populateDummyDeviceInfo(deviceInfo2, "valid.dmap", "card2", "/dev/dev2",
			"./goodMapFile_withoutModules.map");
	populateDummyDeviceInfo(deviceInfo3, "valid.dmap", "card3", "/dev/dev3",
			getCurrentWorkingDirectory()+"/goodMapFile_withoutModules.map");
	std::cout<<getCurrentWorkingDirectory()<<std::endl;

	deviceInfo1.dmap_file_line_nr = 3;
	deviceInfo2.dmap_file_line_nr = 4;
	deviceInfo3.dmap_file_line_nr = 5;

	// we use require here so it is safe to increase and dereference the iterator below
	BOOST_REQUIRE( mapFilePtr->getdmapFileSize() == 3);

	mtca4u::DeviceInfoMap::const_iterator it = mapFilePtr->begin();

	BOOST_CHECK( compareDeviceInfos(deviceInfo1, *(it++)) == true);
	BOOST_CHECK( compareDeviceInfos(deviceInfo2, *(it++)) == true);
	BOOST_CHECK( compareDeviceInfos(deviceInfo3, *(it++)) == true);
}
