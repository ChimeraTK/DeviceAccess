#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Utilities.h"
#include "BackendFactory.h"
#include <boost/filesystem.hpp>

#define VALID_SDM "sdm://./pci:pcieunidummys6;undefined"
#define VALID_SDM_WITH_PARAMS "sdm://./dummy=goodMapFile.map"
#define INVALID_SDM "://./pci:pcieunidummys6;" //no sdm at the start
#define INVALID_SDM_2 "sdm://./pci:pcieunidummys6;;" //more than one semi-colons(;)
#define INVALID_SDM_3 "sdm://./pci::pcieunidummys6;" //more than one colons(:)
#define INVALID_SDM_4 "sdm://./dummy=goodMapFile.map=MapFile.map" //more than one equals to(=)
#define INVALID_SDM_5 "sdm://.pci:pcieunidummys6;" //no slash (/) after host.
#define VALID_PCI_STRING "/dev/mtcadummys0"
#define VALID_DUMMY_STRING "testfile.map"
#define VALID_DUMMY_STRING_2 "testfile.mapp"
#define INVALID_DEVICE_STRING "/mtcadummys0"
#define INVALID_DEVICE_STRING_2 "/dev"
#define INVALID_DEVICE_STRING_3 "testfile.mappp"

using namespace mtca4u;

class UtilitiesTest
{
public:
  static void testParseSdm();
  static void testParseDeviceString();
  static void testcountOccurence();
  static void testIsSdm();
  static void testAliasLookUp();
  static void testFindFirstOfAlias();
};

class UtilitiesTestSuite : public test_suite {
  public:
    UtilitiesTestSuite() : test_suite("Utilities test suite") {
      boost::shared_ptr<UtilitiesTest> utilitiesTest(new UtilitiesTest);
      add(BOOST_TEST_CASE(UtilitiesTest::testParseSdm));
      add(BOOST_TEST_CASE(UtilitiesTest::testParseDeviceString));
      add(BOOST_TEST_CASE(UtilitiesTest::testcountOccurence));
      add(BOOST_TEST_CASE(UtilitiesTest::testIsSdm));
      add(BOOST_TEST_CASE(UtilitiesTest::testAliasLookUp));
      add(BOOST_TEST_CASE(UtilitiesTest::testFindFirstOfAlias));
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "Utilities test suite";
  framework::master_test_suite().add(new UtilitiesTestSuite);
  return NULL;
}

void UtilitiesTest::testParseSdm() {
  Sdm sdm = Utilities::parseSdm(VALID_SDM);
  BOOST_CHECK(sdm._Host == ".");
  BOOST_CHECK(sdm._Interface == "pci");
  BOOST_CHECK(sdm._Instance == "pcieunidummys6");
  BOOST_CHECK(sdm._Parameters.size() == 0);
  BOOST_CHECK(sdm._Protocol == "undefined");

  sdm = Utilities::parseSdm(VALID_SDM_WITH_PARAMS);
  BOOST_CHECK(sdm._Host == ".");
  BOOST_CHECK(sdm._Interface == "dummy");
  BOOST_CHECK(sdm._Parameters.size() == 1);
  BOOST_CHECK(sdm._Parameters.front() == "goodMapFile.map");
  BOOST_CHECK_THROW(Utilities::parseSdm(""),UtilitiesException); //Empty string
  BOOST_CHECK_THROW(Utilities::parseSdm("sdm:"),UtilitiesException); //shorter than sdm:// signature
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM),UtilitiesException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_2),UtilitiesException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_3),UtilitiesException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_4),UtilitiesException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_5),UtilitiesException);
}

void UtilitiesTest::testParseDeviceString() {
  Sdm sdm = Utilities::parseDeviceString(VALID_PCI_STRING);
  BOOST_CHECK(sdm._Interface == "pci");
  sdm = Utilities::parseDeviceString(VALID_DUMMY_STRING);
  BOOST_CHECK(sdm._Interface == "dummy");
  sdm = Utilities::parseDeviceString(VALID_DUMMY_STRING_2);
  BOOST_CHECK(sdm._Interface == "dummy");
  sdm = Utilities::parseDeviceString(INVALID_DEVICE_STRING);
  BOOST_CHECK(sdm._Interface == "");
  sdm = Utilities::parseDeviceString(INVALID_DEVICE_STRING_2);
  BOOST_CHECK(sdm._Interface == "");
  sdm = Utilities::parseDeviceString(INVALID_DEVICE_STRING_3);
  BOOST_CHECK(sdm._Interface == "");

}

void UtilitiesTest::testcountOccurence() {
  BOOST_CHECK(Utilities::countOccurence("this,is;a:test,string",',')==2); //2 commas
  BOOST_CHECK(Utilities::countOccurence("this,is;a:test,string",';')==1); //1 semi-colon
  BOOST_CHECK(Utilities::countOccurence("this,is;a:test,string",':')==1); //1 colon
}

void UtilitiesTest::testIsSdm() {
  BOOST_CHECK(Utilities::isSdm(VALID_SDM) == true);
  BOOST_CHECK(Utilities::isSdm(INVALID_SDM) == false);
  BOOST_CHECK(Utilities::isSdm(VALID_PCI_STRING) == false);
}

void UtilitiesTest::testAliasLookUp() {
  setenv(ENV_VAR_DMAP_FILE, "/usr/local/include/dummies.dmap", 1);
  std::string testFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
  BackendFactory::getInstance().setDMapFilePath(testFilePath);
  std::string uri = Utilities::aliasLookUp("test",testFilePath);
  BOOST_CHECK(uri.empty());
  uri = Utilities::aliasLookUp("DUMMYD0",testFilePath);
  BOOST_CHECK(!uri.empty());
}

void UtilitiesTest::testFindFirstOfAlias() {
  setenv(ENV_VAR_DMAP_FILE, "/usr/local/include/dummies.dmap", 1);
  std::string testFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
  BackendFactory::getInstance().setDMapFilePath(testFilePath);
  std::string dmapfile = Utilities::findFirstOfAlias("test");
  BOOST_CHECK(dmapfile.empty());
  dmapfile = Utilities::findFirstOfAlias("DUMMYD0");
  BOOST_CHECK(!dmapfile.empty());
}

