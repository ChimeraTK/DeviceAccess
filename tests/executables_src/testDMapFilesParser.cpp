#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include "DMapFilesParser.h"
#include "helperFunctions.h"
#include "MapException.h"

class DMapFilesParserTest {
public:
	/** Test the parsing of a file and check the DeviceInfos.
	 *  If the path is not empty it must end with '/'.
	 */
	void testParseFile(std::string pathToDmapFile);
	void testParseEmptyDmapFile();
	void testParseNonExistentDmapFile(std::string dmapFile);
	void testGetMapFile();
	void testGetRegisterInfo();
	void testGetDMapFileSize();
	void testCheckParsedInInfo();
	void testOverloadedStreamOperator();
	void testIteratorBeginEnd();
	void testParsedirInvalidDir();
	void testParseEmptyDirectory();
	void testParseDirectoryWithBlankDMap();
	void testParseDirWithGoodDmaps();
	void testParseDirs();
	void testConstructor();
	void testMapException();
};

class DMapFilesParserTestSuite : public test_suite {
public:
	DMapFilesParserTestSuite() : test_suite("dMapFilesParser class test suite") {

		boost::shared_ptr<DMapFilesParserTest> DMapFilesParserTestPtr(
				new DMapFilesParserTest);

		// test parsing of dmap file in the current directory
		add( BOOST_TEST_CASE( boost::bind(&DMapFilesParserTest::testParseFile,
				DMapFilesParserTestPtr,
				"") ) );
		add( BOOST_TEST_CASE( boost::bind(&DMapFilesParserTest::testParseFile,
				DMapFilesParserTestPtr,
				"dMapDir/") ) );
		add( BOOST_TEST_CASE( boost::bind(&DMapFilesParserTest::testParseFile,
				DMapFilesParserTestPtr,
				getCurrentWorkingDirectory()+"/") ) );
		// test with an empty, existing file
		add( BOOST_CLASS_TEST_CASE( &DMapFilesParserTest::testParseEmptyDmapFile,
				DMapFilesParserTestPtr ) );
		// test with a non-existing file
		add( BOOST_TEST_CASE( boost::bind(&DMapFilesParserTest::testParseNonExistentDmapFile,
				DMapFilesParserTestPtr,
				"notExisting.dmap") ) );
		// special case: file in the root directory. Cannot be there in the test, an probably
		// also not in real life.
		add( BOOST_TEST_CASE( boost::bind(&DMapFilesParserTest::testParseNonExistentDmapFile,
				DMapFilesParserTestPtr,
				"/some.dmap") ) );

		test_case* testGetMapFile = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testGetMapFile, DMapFilesParserTestPtr);
		test_case* testRegisterInfo = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testGetRegisterInfo, DMapFilesParserTestPtr);
		test_case* testGetDMapFileSize =
				BOOST_CLASS_TEST_CASE(&DMapFilesParserTest::testGetDMapFileSize,
						DMapFilesParserTestPtr);
		test_case* testCheckParsedInInfo = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testCheckParsedInInfo, DMapFilesParserTestPtr);
		test_case* testOverloadedStreamOperator = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testOverloadedStreamOperator,
				DMapFilesParserTestPtr);
		test_case* testIteratorBeginEnd = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testIteratorBeginEnd, DMapFilesParserTestPtr);

		test_case* testParsedirInvalidDir = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testParsedirInvalidDir, DMapFilesParserTestPtr);
		test_case* testParseEmptyDirectory = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testParseEmptyDirectory, DMapFilesParserTestPtr);
		test_case* testParseDirectoryWithBlankDMap = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testParseDirectoryWithBlankDMap,
				DMapFilesParserTestPtr);
		test_case* testParseDirWithGoodDmaps =
				BOOST_CLASS_TEST_CASE(&DMapFilesParserTest::testParseDirWithGoodDmaps,
						DMapFilesParserTestPtr);
		test_case* testParseDirs = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testParseDirs, DMapFilesParserTestPtr);

		test_case* testConstructor = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testConstructor, DMapFilesParserTestPtr);
		test_case* testMapException = BOOST_CLASS_TEST_CASE(
				&DMapFilesParserTest::testMapException, DMapFilesParserTestPtr);

		add(testGetMapFile);
		add(testRegisterInfo);
		add(testGetDMapFileSize);
		add(testCheckParsedInInfo);
		add(testOverloadedStreamOperator);
		add(testIteratorBeginEnd);
		add(testParsedirInvalidDir);
		add(testParseEmptyDirectory);
		add(testParseDirectoryWithBlankDMap);
		add(testParseDirWithGoodDmaps);
		add(testParseDirs);
		add(testConstructor);
		add(testMapException);
	}
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value =
			"dMapFilesParser class test suite";
	framework::master_test_suite().add(new DMapFilesParserTestSuite());

	return NULL;
}

void DMapFilesParserTest::testParseFile(std::string pathToDmapFile) {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dmap_file = pathToDmapFile+"valid.dmap";
	std::string path_to_map_file1 = "goodMapFile_withoutModules.map";
	std::string path_to_map_file2 = "./goodMapFile_withoutModules.map";
	std::string path_to_map_file3 = getCurrentWorkingDirectory()+"/goodMapFile_withoutModules.map";
	std::cout<<getCurrentWorkingDirectory()<<std::endl;
	std::cout<<path_to_dmap_file<<std::endl;
	filesParser.parse_file(path_to_dmap_file);
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo3;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo4;

	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo3;

	populateDummyDeviceInfo(expectedDeviceInfo1, path_to_dmap_file, "card1",
			"/dev/dev1", path_to_map_file1);
	populateDummyDeviceInfo(expectedDeviceInfo2, path_to_dmap_file, "card2",
			"/dev/dev2", path_to_map_file2);
	populateDummyDeviceInfo(expectedDeviceInfo3, path_to_dmap_file, "card3",
			"/dev/dev3", path_to_map_file3);

	expectedDeviceInfo1.dmap_file_line_nr = 3;
	expectedDeviceInfo2.dmap_file_line_nr = 4;
	expectedDeviceInfo3.dmap_file_line_nr = 5;

	filesParser.getdMapFileElem(0, reterievedDeviceInfo1);
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1,
			reterievedDeviceInfo1) == true);

	filesParser.getdMapFileElem(1, reterievedDeviceInfo2);
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2,
			reterievedDeviceInfo2) == true);

	filesParser.getdMapFileElem(2, reterievedDeviceInfo3);

	std::cout<<expectedDeviceInfo3.dmap_file_name<<std::endl;
	std::cout<<reterievedDeviceInfo3.dmap_file_name<<std::endl;

	std::cout<<expectedDeviceInfo3.map_file_name<<std::endl;
	std::cout<<reterievedDeviceInfo3.map_file_name<<std::endl;

	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo3,
			reterievedDeviceInfo3) == true);

	BOOST_CHECK_THROW(filesParser.getdMapFileElem(3, reterievedDeviceInfo4),
			mtca4u::LibMapException);

	try {
		filesParser.getdMapFileElem(3, reterievedDeviceInfo4);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}

	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo5 =
			filesParser.getdMapFileElem("card2");

	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2,
			reterievedDeviceInfo5) == true);

	BOOST_CHECK_THROW(filesParser.getdMapFileElem("card_not_present"),
			mtca4u::DMapFileParserException);
	try {
		filesParser.getdMapFileElem("card_not_present");
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}

	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo6;
	filesParser.getdMapFileElem("card2", reterievedDeviceInfo6);
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2,
			reterievedDeviceInfo6) == true);
}

void DMapFilesParserTest::testParseEmptyDmapFile() {
	mtca4u::DMapFilesParser filesParser;
	std::string dmapFileName("empty.dmap");

	BOOST_CHECK_THROW(filesParser.parse_file(dmapFileName),
			mtca4u::DMapFileParserException);
	try {
		filesParser.parse_file(dmapFileName);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DMAP_DATA);
	}
}

void DMapFilesParserTest::testParseNonExistentDmapFile(std::string dmapFile) {
	mtca4u::DMapFilesParser filesParser;

	BOOST_CHECK_THROW(filesParser.parse_file(dmapFile),
			mtca4u::DMapFileParserException);
	try {
		filesParser.parse_file(dmapFile);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
	}
}

void DMapFilesParserTest::testGetMapFile() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dmap_file = "dMapDir/valid.dmap";
	filesParser.parse_file(path_to_dmap_file);

	boost::shared_ptr<mtca4u::RegisterInfoMap> map_file_for_card1;
	boost::shared_ptr<mtca4u::RegisterInfoMap> map_file_for_card3;

	map_file_for_card1 = filesParser.getMapFile("card1");
	map_file_for_card3 = filesParser.getMapFile("card3");

	// Card 1 elements
	mtca4u::RegisterInfoMap::RegisterInfo card1_RegisterInfoent1("WORD_FIRMWARE", 0x00000001,
			0x00000000, 0x00000004, 0x0,
			32, 0, true, 5);
	mtca4u::RegisterInfoMap::RegisterInfo card1_RegisterInfoent2("WORD_COMPILATION", 0x00000001,
			0x00000004, 0x00000004, 0x00000000,
			32, 0, true, 6);
	mtca4u::RegisterInfoMap::RegisterInfo card1_RegisterInfoent3("WORD_STATUS", 0x00000001,
			0x00000008, 0x00000004, 0x00000000,
			32, 0, true, 7);
	mtca4u::RegisterInfoMap::RegisterInfo card1_RegisterInfoent4("WORD_USER1", 0x00000001,
			0x0000000C, 0x00000004, 0x00000000,
			32, 0, true, 8);
	mtca4u::RegisterInfoMap::RegisterInfo card1_RegisterInfoent5("WORD_USER2", 0x00000001,
			0x00000010, 0x00000004, 0x00000000,
			32, 0, 0, 9);

	mtca4u::RegisterInfoMap::RegisterInfo* ptrList[5];
	ptrList[0] = &card1_RegisterInfoent1;
	ptrList[1] = &card1_RegisterInfoent2;
	ptrList[2] = &card1_RegisterInfoent3;
	ptrList[3] = &card1_RegisterInfoent4;
	ptrList[4] = &card1_RegisterInfoent5;
	int index;
	mtca4u::RegisterInfoMap::iterator it;
	for (it = map_file_for_card1->begin(), index = 0;
			it != map_file_for_card1->end(); ++it, ++index) {
		BOOST_CHECK((compareRegisterInfoents(*ptrList[index], *it)) == true);
	}

	// Card 3 elements
	mtca4u::RegisterInfoMap::RegisterInfo card3_RegisterInfoent1("WORD_FIRMWARE", 0x00000001,
			0x00000000, 0x00000004, 0x0,
			32, 0, true, 5);
	mtca4u::RegisterInfoMap::RegisterInfo card3_RegisterInfoent2("WORD_COMPILATION", 0x00000001,
			0x00000004, 0x00000004, 0x00000000,
			32, 0, true, 6);
	mtca4u::RegisterInfoMap::RegisterInfo card3_RegisterInfoent3("WORD_STATUS", 0x00000001,
			0x00000008, 0x00000004, 0x00000000,
			32, 0, true, 7);
	mtca4u::RegisterInfoMap::RegisterInfo card3_RegisterInfoent4("WORD_USER1", 0x00000001,
			0x0000000C, 0x00000004, 0x00000000,
			32, 0, true, 8);
	mtca4u::RegisterInfoMap::RegisterInfo card3_RegisterInfoent5("WORD_USER2", 0x00000001,
			0x00000010, 0x00000004, 0x00000000,
			32, 0, 0, 9);

	ptrList[0] = &card3_RegisterInfoent1;
	ptrList[1] = &card3_RegisterInfoent2;
	ptrList[2] = &card3_RegisterInfoent3;
	ptrList[3] = &card3_RegisterInfoent4;
	ptrList[4] = &card3_RegisterInfoent5;
	for (it = map_file_for_card3->begin(), index = 0;
			it != map_file_for_card3->end(); ++it, ++index) {
		BOOST_CHECK((compareRegisterInfoents(*ptrList[index], *it)) == true);
	}

	BOOST_CHECK_THROW(filesParser.getMapFile("card_unknown"),
			mtca4u::DMapFileParserException);
	try {
		filesParser.getMapFile("card_unknown");
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}
}

void DMapFilesParserTest::testGetRegisterInfo() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dmap_file = "dMapDir/valid.dmap";
	filesParser.parse_file(path_to_dmap_file);

	std::string retrivedDeviceFileName;
	mtca4u::RegisterInfoMap::RegisterInfo retrivedRegisterInfo;
	mtca4u::RegisterInfoMap::RegisterInfo RegisterInfoent3("WORD_STATUS", 0x00000001, 0x00000008,
			0x00000004, 0x00000000, 32, 0, true, 7);
	filesParser.getRegisterInfo("card1", "WORD_STATUS", retrivedDeviceFileName,
			retrivedRegisterInfo);
	BOOST_CHECK(retrivedDeviceFileName == "/dev/dev1");
	BOOST_CHECK(compareRegisterInfoents(retrivedRegisterInfo, RegisterInfoent3) == true);

	filesParser.getRegisterInfo("card3", "WORD_STATUS", retrivedDeviceFileName,
			retrivedRegisterInfo);

	BOOST_CHECK(retrivedDeviceFileName == "/dev/dev3");
	BOOST_CHECK(compareRegisterInfoents(retrivedRegisterInfo, RegisterInfoent3) == true);

	BOOST_CHECK_THROW(
			filesParser.getRegisterInfo("card_unknown", "WORD_STATUS",
					retrivedDeviceFileName, retrivedRegisterInfo),
					mtca4u::DMapFileParserException);
	try {
		filesParser.getRegisterInfo("card_unknown", "WORD_STATUS",
				retrivedDeviceFileName, retrivedRegisterInfo);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}

	mtca4u::DMapFilesParser filesParser2;
	path_to_dmap_file = "dMapDir/oneDevice.dmap";
	filesParser2.parse_file(path_to_dmap_file);
	filesParser2.getRegisterInfo("", "WORD_STATUS", retrivedDeviceFileName,
			retrivedRegisterInfo);
	BOOST_CHECK(retrivedDeviceFileName == "/dev/dev1");
	BOOST_CHECK(compareRegisterInfoents(retrivedRegisterInfo, RegisterInfoent3) == true);

	uint32_t retrivedElemNr;
	uint32_t retrivedOffset;
	uint32_t retrivedRegSize;
	uint32_t retrivedRegBar;

	filesParser.getRegisterInfo("card2", "WORD_STATUS", retrivedDeviceFileName,
			retrivedElemNr, retrivedOffset, retrivedRegSize,
			retrivedRegBar);
	BOOST_CHECK(retrivedDeviceFileName == "/dev/dev2");
	BOOST_CHECK(retrivedElemNr == 1);
	BOOST_CHECK(retrivedOffset == 0x00000008);
	BOOST_CHECK(retrivedRegSize == 0x00000004);
	BOOST_CHECK(retrivedRegBar == 0x00000000);

	filesParser2.getRegisterInfo("", "WORD_STATUS", retrivedDeviceFileName,
			retrivedElemNr, retrivedOffset, retrivedRegSize,
			retrivedRegBar);
	BOOST_CHECK(retrivedDeviceFileName == "/dev/dev1");
	BOOST_CHECK(retrivedElemNr == 1);
	BOOST_CHECK(retrivedOffset == 0x00000008);
	BOOST_CHECK(retrivedRegSize == 0x00000004);
	BOOST_CHECK(retrivedRegBar == 0x00000000);

	BOOST_CHECK_THROW(
			filesParser.getRegisterInfo(
					"unknown_card", "WORD_STATUS", retrivedDeviceFileName, retrivedElemNr,
					retrivedOffset, retrivedRegSize, retrivedRegBar),
					mtca4u::DMapFileParserException);
	try {
		filesParser.getRegisterInfo(
				"unknown_card", "WORD_STATUS", retrivedDeviceFileName, retrivedElemNr,
				retrivedOffset, retrivedRegSize, retrivedRegBar);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}
}

void DMapFilesParserTest::testGetDMapFileSize() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dmap_file = "dMapDir/valid.dmap";
	filesParser.parse_file(path_to_dmap_file);

	BOOST_CHECK(filesParser.getdMapFileSize() == 3);
}

void DMapFilesParserTest::testCheckParsedInInfo() {
	mtca4u::DMapFilesParser filesParser;
	mtca4u::DMapFilesParser filesParser1;
	std::string path_to_dmap_file = "dMapDir/NonUniqueCardName.dmap";
	filesParser.parse_file(path_to_dmap_file);
	filesParser1.parse_file("dMapDir/oneDevice.dmap");

	mtca4u::DeviceInfoMap::ErrorList dmap_err_list;
	mtca4u::RegisterInfoMap::ErrorList map_err_list;

	bool returnValue =
			filesParser1.check(mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
					mtca4u::RegisterInfoMap::ErrorList::ErrorElem::WARNING,
					dmap_err_list, map_err_list);
	BOOST_CHECK(returnValue == true);

	bool status =
			filesParser.check(mtca4u::DeviceInfoMap::ErrorList::ErrorElem::ERROR,
					mtca4u::RegisterInfoMap::ErrorList::ErrorElem::WARNING,
					dmap_err_list, map_err_list);

	BOOST_CHECK(status == false);
	int numberOfIncorrectLinesInFile = dmap_err_list.errors.size();
	BOOST_CHECK(numberOfIncorrectLinesInFile == 1);
	std::list<mtca4u::DeviceInfoMap::ErrorList::ErrorElem>::iterator errorIterator =
			dmap_err_list.errors.begin();
	BOOST_CHECK(
			(errorIterator->err_dev_1.dev_name == errorIterator->err_dev_2.dev_name));

	BOOST_CHECK(map_err_list.errors.size() == 2);
	std::list<mtca4u::RegisterInfoMap::ErrorList::ErrorElem>::iterator mapErrIt;
	mapErrIt = map_err_list.errors.begin();

	bool areNonUniqueRegistersPresent =
			((mapErrIt->err_reg_1.reg_name == mapErrIt->err_reg_2.reg_name) &&
					((mapErrIt->err_reg_1.reg_address != mapErrIt->err_reg_2.reg_address) ||
							(mapErrIt->err_reg_1.reg_bar != mapErrIt->err_reg_2.reg_bar) ||
							(mapErrIt->err_reg_1.reg_elem_nr != mapErrIt->err_reg_2.reg_elem_nr) ||
							(mapErrIt->err_reg_1.reg_size != mapErrIt->err_reg_2.reg_size)));
	BOOST_CHECK(areNonUniqueRegistersPresent);
}

void DMapFilesParserTest::testOverloadedStreamOperator() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dmap_file = "dMapDir/valid.dmap";
	filesParser.parse_file(path_to_dmap_file);

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo3;

	populateDummyDeviceInfo(deviceInfo1, path_to_dmap_file, "card1",
			"/dev/dev1", "goodMapFile_withoutModules.map");
	populateDummyDeviceInfo(deviceInfo2, path_to_dmap_file, "card2",
			"/dev/dev2", "./goodMapFile_withoutModules.map");
	populateDummyDeviceInfo(deviceInfo3, path_to_dmap_file, "card3",
			"/dev/dev3", getCurrentWorkingDirectory()+"/goodMapFile_withoutModules.map");

	deviceInfo1.dmap_file_line_nr = 3;
	deviceInfo2.dmap_file_line_nr = 4;
	deviceInfo3.dmap_file_line_nr = 5;

	std::stringstream expected_file_stream;
	expected_file_stream << deviceInfo1 << std::endl;
	expected_file_stream << deviceInfo2 << std::endl;
	expected_file_stream << deviceInfo3 << std::endl;

	std::stringstream actual_file_stream;
	actual_file_stream << filesParser;

	BOOST_CHECK(expected_file_stream.str() == actual_file_stream.str());
}

void DMapFilesParserTest::testIteratorBeginEnd() {
	mtca4u::DMapFilesParser filesParser;
	mtca4u::DMapFilesParser const& constfilesParser = filesParser;
	std::string path_to_dmap_file = "dMapDir/valid.dmap";
	filesParser.parse_file(path_to_dmap_file);

	std::string currentWrkingDir = getCurrentWorkingDirectory();

	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo deviceInfo3;

	populateDummyDeviceInfo(deviceInfo1, path_to_dmap_file, "card1",
			"/dev/dev1", "goodMapFile_withoutModules.map");
	populateDummyDeviceInfo(deviceInfo2, path_to_dmap_file, "card2",
			"/dev/dev2", "./goodMapFile_withoutModules.map");
	// the third path is absolute, does not change with the location of the dmap file
	populateDummyDeviceInfo(deviceInfo3, path_to_dmap_file, "card3",
			"/dev/dev3", getCurrentWorkingDirectory() + "/goodMapFile_withoutModules.map");

	deviceInfo1.dmap_file_line_nr = 3;
	deviceInfo2.dmap_file_line_nr = 4;
	deviceInfo3.dmap_file_line_nr = 5;

	mtca4u::DeviceInfoMap::DeviceInfo* tmpArray1[3];
	tmpArray1[0] = &deviceInfo1;
	tmpArray1[1] = &deviceInfo2;
	tmpArray1[2] = &deviceInfo3;


	std::string s1 = currentWrkingDir + "/" + "dMapDir/goodMapFile_withoutModules.map";
	std::string s2 = currentWrkingDir + "/" + "dMapDir/./goodMapFile_withoutModules.map";
	// the third path is absolute, does not change with the location of the dmap file
	std::string s3 = currentWrkingDir + "/" + "goodMapFile_withoutModules.map";
	std::string* tmpArray2[3];
	tmpArray2[0] = &s1;
	tmpArray2[1] = &s2;
	tmpArray2[2] = &s3;

	std::vector<std::pair<mtca4u::DeviceInfoMap::DeviceInfo,
	mtca4u::ptrmapFile> >::iterator iter;
	std::vector<std::pair<mtca4u::DeviceInfoMap::DeviceInfo,
	mtca4u::ptrmapFile> >::const_iterator const_iter;
	uint8_t i;
	for (iter = filesParser.begin(), i = 0;
			(iter != filesParser.end()) && (i < 3);
			i++, iter++) {
		BOOST_CHECK(compareDeviceInfos(*tmpArray1[i], (*iter).first) == true);
		BOOST_CHECK(*tmpArray2[i] == (*iter).second->getMapFileName());
	}
	for (const_iter = constfilesParser.begin(), i = 0;
			(const_iter != constfilesParser.end()) && (i < 3);
			i++, const_iter++) {
		BOOST_CHECK(compareDeviceInfos(*tmpArray1[i], (*const_iter).first) ==
				true);
		BOOST_CHECK(*tmpArray2[i] == (*const_iter).second->getMapFileName());
	}
}

void DMapFilesParserTest::testParsedirInvalidDir() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dir = "NonExistentDir";

	BOOST_CHECK_THROW(filesParser.parse_dir(path_to_dir),
			mtca4u::DMapFileParserException)
	try {
		filesParser.parse_dir(path_to_dir);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
	}
}

void DMapFilesParserTest::testParseEmptyDirectory() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dir = "EmptyDir";

	BOOST_CHECK_THROW(filesParser.parse_dir(path_to_dir),
			mtca4u::DMapFileParserException)
	try {
		filesParser.parse_dir(path_to_dir);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DMAP_DATA);
	}
}

void DMapFilesParserTest::testParseDirectoryWithBlankDMap() {
	mtca4u::DMapFilesParser filesParser;
	std::string path_to_dir = "./BlankFiles";

	BOOST_CHECK_THROW(filesParser.parse_dir(path_to_dir),
			mtca4u::DMapFileParserException)
	try {
		filesParser.parse_dir(path_to_dir);
	}
	catch (mtca4u::DMapFileParserException& exception) {
		BOOST_CHECK(exception.getID() ==
				mtca4u::LibMapException::EX_NO_DMAP_DATA);
	}
}

void DMapFilesParserTest::testParseDirWithGoodDmaps() {
	mtca4u::DMapFilesParser filesParser;
	filesParser.parse_dir("./GoodDmapDir");

	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo3;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo4;

	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo2;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo3;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo4;

	populateDummyDeviceInfo(expectedDeviceInfo1, "./GoodDmapDir/first.dmap",
			"card1", "/dev/dev1", "./mapFile1.map");
	populateDummyDeviceInfo(expectedDeviceInfo2, "./GoodDmapDir/second.dmap",
			"card2", "/dev/dev2", "./mapFile2.map");

	populateDummyDeviceInfo(expectedDeviceInfo3, "./GoodDmapDir/second.dmap",
			"card3", "/dev/dev3", "./mapFile2.map");
	populateDummyDeviceInfo(expectedDeviceInfo4, "./GoodDmapDir/first.dmap",
			"card4", "/dev/dev4", "mtcadummy_withoutModules.map");

	expectedDeviceInfo1.dmap_file_line_nr = 3;
	expectedDeviceInfo2.dmap_file_line_nr = 1;
	expectedDeviceInfo3.dmap_file_line_nr = 2;
	expectedDeviceInfo4.dmap_file_line_nr = 4;

	reterievedDeviceInfo1 = filesParser.getdMapFileElem("card1");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1,
			reterievedDeviceInfo1) == true);

	reterievedDeviceInfo2 = filesParser.getdMapFileElem("card2");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2,
			reterievedDeviceInfo2) == true);

	reterievedDeviceInfo3 = filesParser.getdMapFileElem("card3");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo3,
			reterievedDeviceInfo3) == true);

	reterievedDeviceInfo4 = filesParser.getdMapFileElem("card4");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo4,
			reterievedDeviceInfo4) == true);
}

void DMapFilesParserTest::testParseDirs() {
	std::vector<std::string> a;
	a.push_back("./GoodDmapDir");
	a.push_back("./BlankFiles");

	mtca4u::DMapFilesParser filesParser;
	filesParser.parse_dirs(a);

	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo2;

	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo2;

	populateDummyDeviceInfo(expectedDeviceInfo1, "./GoodDmapDir/first.dmap",
			"card1", "/dev/dev1", "./mapFile1.map");
	populateDummyDeviceInfo(expectedDeviceInfo2, "./GoodDmapDir/second.dmap",
			"card2", "/dev/dev2", "./mapFile2.map");

	expectedDeviceInfo1.dmap_file_line_nr = 3;
	expectedDeviceInfo2.dmap_file_line_nr = 1;

	reterievedDeviceInfo1 = filesParser.getdMapFileElem("card1");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1,
			reterievedDeviceInfo1) == true);

	reterievedDeviceInfo2 = filesParser.getdMapFileElem("card2");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo2,
			reterievedDeviceInfo2) == true);
}

inline void DMapFilesParserTest::testConstructor() {
	mtca4u::DMapFilesParser filesParser("./GoodDmapDir");

	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo reterievedDeviceInfo3;

	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo1;
	mtca4u::DeviceInfoMap::DeviceInfo expectedDeviceInfo3;

	populateDummyDeviceInfo(expectedDeviceInfo1, "./GoodDmapDir/first.dmap",
			"card1", "/dev/dev1", "./mapFile1.map");
	populateDummyDeviceInfo(expectedDeviceInfo3, "./GoodDmapDir/second.dmap",
			"card3", "/dev/dev3", "./mapFile2.map");

	expectedDeviceInfo1.dmap_file_line_nr = 3;
	expectedDeviceInfo3.dmap_file_line_nr = 2;

	reterievedDeviceInfo1 = filesParser.getdMapFileElem("card1");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo1,
			reterievedDeviceInfo1) == true);

	reterievedDeviceInfo3 = filesParser.getdMapFileElem("card3");
	BOOST_CHECK(compareDeviceInfos(expectedDeviceInfo3,
			reterievedDeviceInfo3) == true);
}

void DMapFilesParserTest::testMapException () {
	BOOST_CHECK_THROW(mtca4u::DMapFilesParser filesParser("./emptyMapFile"),
			mtca4u::MapFileException);

	try{
		mtca4u::DMapFilesParser filesParser("./emptyMapFile");
	} catch(mtca4u::LibMapException& ex) {
		BOOST_CHECK(ex.getID() == mtca4u::LibMapException::EX_CANNOT_OPEN_MAP_FILE);
	}

}
