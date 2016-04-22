#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Utilities.h"
#include "BackendFactory.h"
#include "DeviceInfoMap.h"
#include "MapException.h"

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
  static void testgetAliasList();
};

class UtilitiesTestSuite : public test_suite {
  public:
    UtilitiesTestSuite() : test_suite("Utilities test suite") {
      BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
      boost::shared_ptr<UtilitiesTest> utilitiesTest(new UtilitiesTest);
      add(BOOST_TEST_CASE(UtilitiesTest::testParseSdm));
      add(BOOST_TEST_CASE(UtilitiesTest::testParseDeviceString));
      add(BOOST_TEST_CASE(UtilitiesTest::testcountOccurence));
      add(BOOST_TEST_CASE(UtilitiesTest::testIsSdm));
      add(BOOST_TEST_CASE(UtilitiesTest::testAliasLookUp));
      add(BOOST_TEST_CASE(UtilitiesTest::testFindFirstOfAlias));
      add(BOOST_TEST_CASE(UtilitiesTest::testgetAliasList));
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
  BOOST_CHECK_THROW(Utilities::parseSdm(""),SdmUriParseException); //Empty string
  BOOST_CHECK_THROW(Utilities::parseSdm("sdm:"),SdmUriParseException); //shorter than sdm:// signature
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM),SdmUriParseException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_2),SdmUriParseException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_3),SdmUriParseException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_4),SdmUriParseException);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_5),SdmUriParseException);
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
  std::string testFilePath = TEST_DMAP_FILE_PATH;
  BOOST_CHECK_THROW(Utilities::aliasLookUp("test",testFilePath), LibMapException);
  auto deviceInfo = Utilities::aliasLookUp("DUMMYD0",testFilePath);
  BOOST_CHECK(deviceInfo.deviceName =="DUMMYD0");
}

void UtilitiesTest::testFindFirstOfAlias() {
  setenv(ENV_VAR_DMAP_FILE, "/usr/local/include/dummies.dmap", 1);
  std::string testFilePath = TEST_DMAP_FILE_PATH;
  BackendFactory::getInstance().setDMapFilePath(testFilePath);
  BOOST_CHECK_THROW(Utilities::findFirstOfAlias("test"), LibMapException);
  auto dmapfile = Utilities::findFirstOfAlias("DUMMYD0");
  BOOST_CHECK(!dmapfile.empty());
}

void UtilitiesTest::testgetAliasList() {
  auto initialDmapFile = mtca4u::getDMapFilePath();

  mtca4u::setDMapFilePath("");
  BOOST_CHECK_THROW(Utilities::getAliasList(), DeviceException);

  // entries in dummies.dmap when this was written
  std::vector<std::string> expectedListOfAliases{
    "PCIE1",   "PCIE0",   "PCIE2",    "PCIE3",     "PCIE0",
    "FAKE0",   "FAKE1",   "FAKE3",    "DUMMYD0",   "DUMMYD1",  "DUMMYD2",
    "example", "DUMMYD9", "mskrebot", "mskrebot1", "OLD_PCIE",
    "SEQUENCES", "MIXED_SEQUENCES", "INVALID_SEQUENCES", "PCIE_DOUBLEMAP",
    "REBOT_DOUBLEMAP"
  };

  mtca4u::setDMapFilePath("./dummies.dmap");
  auto returnedListOfAliases = Utilities::getAliasList();
  mtca4u::setDMapFilePath(initialDmapFile);

  int index = 0;
  BOOST_CHECK(returnedListOfAliases.size() == expectedListOfAliases.size());
  for(auto alias: expectedListOfAliases){
    BOOST_CHECK(alias == returnedListOfAliases.at(index++));
  }
}
