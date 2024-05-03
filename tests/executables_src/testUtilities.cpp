// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DeviceTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "DeviceInfoMap.h"
#include "Exception.h"
#include "SupportedUserTypes.h"
#include "Utilities.h"

#define VALID_SDM "sdm://./pci:pcieunidummys6;undefined"
#define VALID_SDM_WITH_PARAMS "sdm://./dummy=goodMapFile.map"
#define INVALID_SDM "://./pci:pcieunidummys6;"                    // no sdm at the start
#define INVALID_SDM_2 "sdm://./pci:pcieunidummys6;;"              // more than one semi-colons(;)
#define INVALID_SDM_3 "sdm://./pci::pcieunidummys6;"              // more than one colons(:)
#define INVALID_SDM_4 "sdm://./dummy=goodMapFile.map=MapFile.map" // more than one equals to(=)
#define INVALID_SDM_5 "sdm://.pci:pcieunidummys6;"                // no slash (/) after host.
#define VALID_PCI_STRING "/dev/mtcadummys0"
#define VALID_DUMMY_STRING "testfile.map"
#define VALID_DUMMY_STRING_2 "testfile.mapp"
#define INVALID_DEVICE_STRING "/mtcadummys0"
#define INVALID_DEVICE_STRING_2 "/dev"
#define INVALID_DEVICE_STRING_3 "testfile.mappp"

using namespace ChimeraTK;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParseCDD) {
  {
    // check standard case
    auto r = Utilities::parseDeviceDesciptor("(myBackendType:some/weired*address 234?par1=someValue with "
                                             "spaces&map=file)");
    BOOST_CHECK_EQUAL(r.backendType, "myBackendType");
    BOOST_CHECK_EQUAL(r.address, "some/weired*address 234");
    BOOST_CHECK_EQUAL(r.parameters.size(), 2);
    BOOST_CHECK_EQUAL(r.parameters["par1"], "someValue with spaces");
    BOOST_CHECK_EQUAL(r.parameters["map"], "file");
  }
  {
    // check proper trimming
    auto r = Utilities::parseDeviceDesciptor(" ( myBackendType    :     some/weired*address 234 ?   par1 = "
                                             "someValue with spaces & map  =   file  )   ");
    BOOST_CHECK_EQUAL(r.backendType, "myBackendType");
    BOOST_CHECK_EQUAL(r.address, "some/weired*address 234");
    BOOST_CHECK_EQUAL(r.parameters.size(), 2);
    BOOST_CHECK_EQUAL(r.parameters["par1"], "someValue with spaces");
    BOOST_CHECK_EQUAL(r.parameters["map"], "file");
  }
  {
    // check only backend type
    auto r = Utilities::parseDeviceDesciptor("(someStrangeBackendType)");
    BOOST_CHECK_EQUAL(r.backendType, "someStrangeBackendType");
    BOOST_CHECK_EQUAL(r.address, "");
    BOOST_CHECK_EQUAL(r.parameters.size(), 0);
  }
  {
    // check only backend type with address
    auto r = Utilities::parseDeviceDesciptor("(pci:pcieunis6)");
    BOOST_CHECK_EQUAL(r.backendType, "pci");
    BOOST_CHECK_EQUAL(r.address, "pcieunis6");
    BOOST_CHECK_EQUAL(r.parameters.size(), 0);
  }
  {
    // check explicitly empty parameter list
    auto r = Utilities::parseDeviceDesciptor("(pci:pcieunis6?)");
    BOOST_CHECK_EQUAL(r.backendType, "pci");
    BOOST_CHECK_EQUAL(r.address, "pcieunis6");
    BOOST_CHECK_EQUAL(r.parameters.size(), 0);
  }
  {
    // check explicitly empty parameter list with more empty parameters
    auto r = Utilities::parseDeviceDesciptor("(pci:pcieunis6?&&)");
    BOOST_CHECK_EQUAL(r.backendType, "pci");
    BOOST_CHECK_EQUAL(r.address, "pcieunis6");
    BOOST_CHECK_EQUAL(r.parameters.size(), 0);
  }
  {
    // check only backend type with parameters
    auto r = Utilities::parseDeviceDesciptor("(logicalNameMapper?map=myMapFile.xlmap)");
    BOOST_CHECK_EQUAL(r.backendType, "logicalNameMapper");
    BOOST_CHECK_EQUAL(r.address, "");
    BOOST_CHECK_EQUAL(r.parameters.size(), 1);
    BOOST_CHECK_EQUAL(r.parameters["map"], "myMapFile.xlmap");
  }
  {
    // check explicitly empty address
    auto r = Utilities::parseDeviceDesciptor("(logicalNameMapper:?map=myMapFile.xlmap)");
    BOOST_CHECK_EQUAL(r.backendType, "logicalNameMapper");
    BOOST_CHECK_EQUAL(r.address, "");
    BOOST_CHECK_EQUAL(r.parameters.size(), 1);
    BOOST_CHECK_EQUAL(r.parameters["map"], "myMapFile.xlmap");
  }
  {
    // check explicitly empty parameters
    auto r = Utilities::parseDeviceDesciptor("(logicalNameMapper?&map=myMapFile.xlmap& &a=b&)");
    BOOST_CHECK_EQUAL(r.backendType, "logicalNameMapper");
    BOOST_CHECK_EQUAL(r.address, "");
    BOOST_CHECK_EQUAL(r.parameters.size(), 2);
    BOOST_CHECK_EQUAL(r.parameters["map"], "myMapFile.xlmap");
    BOOST_CHECK_EQUAL(r.parameters["a"], "b");
  }
  {
    // check parameter value with equal sign
    auto r = Utilities::parseDeviceDesciptor("(x?a=b=c)");
    BOOST_CHECK_EQUAL(r.backendType, "x");
    BOOST_CHECK_EQUAL(r.address, "");
    BOOST_CHECK_EQUAL(r.parameters.size(), 1);
    BOOST_CHECK_EQUAL(r.parameters["a"], "b=c");
  }
  {
    // check escaping special characters
    auto r = Utilities::parseDeviceDesciptor("(x:address\\?withQuestionmark?para=value\\&with\\&ampersand\\&&x="
                                             "y\\\\&y=\\))");
    BOOST_CHECK_EQUAL(r.backendType, "x");
    BOOST_CHECK_EQUAL(r.address, "address?withQuestionmark");
    BOOST_CHECK_EQUAL(r.parameters.size(), 3);
    BOOST_CHECK_EQUAL(r.parameters["para"], "value&with&ampersand&");
    BOOST_CHECK_EQUAL(r.parameters["x"], "y\\");
    BOOST_CHECK_EQUAL(r.parameters["y"], ")");
  }
  {
    // check nesting CDDs
    auto r = Utilities::parseDeviceDesciptor("(nested:(pci:pcieunis6?map=dummy.map)?"
                                             "anotherCdd=with(dummycdd)otherText)");
    BOOST_CHECK_EQUAL(r.backendType, "nested");
    BOOST_CHECK_EQUAL(r.address, "(pci:pcieunis6?map=dummy.map)");
    BOOST_CHECK_EQUAL(r.parameters.size(), 1);
    BOOST_CHECK_EQUAL(r.parameters["anotherCdd"], "with(dummycdd)otherText");
  }
  // check exceptions
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor(""), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("noParantheses"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(  )"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(backend)ExtraChars"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(:address)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(bad_backend_name)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(x?keyNoValue)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(x?=valueNoKey)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(x?bad*key=value)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(x?key=value&key=duplicateKey)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(unmatchedParentheses"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      Utilities::parseDeviceDesciptor("(another:Unmatched?parentheses=in(aValue)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      Utilities::parseDeviceDesciptor("(another:Unmatched?parentheses=in)aValue)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseDeviceDesciptor("(badEscaping:a\\bc)"), ChimeraTK::logic_error);
}
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParseSdm) {
  Sdm sdm = Utilities::parseSdm(VALID_SDM);
  BOOST_CHECK(sdm.host == ".");
  BOOST_CHECK(sdm.interface == "pci");
  BOOST_CHECK(sdm.instance == "pcieunidummys6");
  BOOST_CHECK(sdm.parameters.size() == 0);
  BOOST_CHECK(sdm.protocol == "undefined");

  sdm = Utilities::parseSdm(VALID_SDM_WITH_PARAMS);
  BOOST_CHECK(sdm.host == ".");
  BOOST_CHECK(sdm.interface == "dummy");
  BOOST_CHECK(sdm.parameters.size() == 1);
  BOOST_CHECK(sdm.parameters.front() == "goodMapFile.map");
  BOOST_CHECK_THROW(Utilities::parseSdm(""),
      ChimeraTK::logic_error); // Empty string
  BOOST_CHECK_THROW(Utilities::parseSdm("sdm:"),
      ChimeraTK::logic_error); // shorter than sdm:// signature
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_2), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_3), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_4), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(Utilities::parseSdm(INVALID_SDM_5), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParseDeviceString) {
  Sdm sdm = Utilities::parseDeviceString(VALID_PCI_STRING);
  BOOST_CHECK(sdm.interface == "pci");
  sdm = Utilities::parseDeviceString(VALID_DUMMY_STRING);
  BOOST_CHECK(sdm.interface == "dummy");
  sdm = Utilities::parseDeviceString(VALID_DUMMY_STRING_2);
  BOOST_CHECK(sdm.interface == "dummy");
  sdm = Utilities::parseDeviceString(INVALID_DEVICE_STRING);
  BOOST_CHECK(sdm.interface == "");
  sdm = Utilities::parseDeviceString(INVALID_DEVICE_STRING_2);
  BOOST_CHECK(sdm.interface == "");
  sdm = Utilities::parseDeviceString(INVALID_DEVICE_STRING_3);
  BOOST_CHECK(sdm.interface == "");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testcountOccurence) {
  BOOST_CHECK(Utilities::countOccurence("this,is;a:test,string", ',') == 2); // 2 commas
  BOOST_CHECK(Utilities::countOccurence("this,is;a:test,string", ';') == 1); // 1 semi-colon
  BOOST_CHECK(Utilities::countOccurence("this,is;a:test,string", ':') == 1); // 1 colon
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testIsSdm) {
  BOOST_CHECK(Utilities::isSdm(VALID_SDM) == true);
  BOOST_CHECK(Utilities::isSdm(INVALID_SDM) == false);
  BOOST_CHECK(Utilities::isSdm(VALID_PCI_STRING) == false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAliasLookUp) {
  std::string testFilePath = TEST_DMAP_FILE_PATH;
  BOOST_CHECK_THROW(Utilities::aliasLookUp("test", testFilePath), ChimeraTK::logic_error);
  auto deviceInfo = Utilities::aliasLookUp("DUMMYD0", testFilePath);
  BOOST_CHECK(deviceInfo.deviceName == "DUMMYD0");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testgetAliasList) {
  auto initialDmapFile = ChimeraTK::getDMapFilePath();

  ChimeraTK::setDMapFilePath("");
  BOOST_CHECK_THROW(Utilities::getAliasList(), ChimeraTK::logic_error);

  // entries in dummies.dmap when this was written
  std::vector<std::string> expectedListOfAliases{"PCIE1", "PCIE0", "PCIE2", "PCIE3", "PCIE0", "DUMMYD0", "DUMMYD1",
      "DUMMYD2", "DUMMYD3", "DUMMYD9", "PERFTEST", "mskrebot", "mskrebot1", "OLD_PCIE", "SEQUENCES",
      "INVALID_SEQUENCES", "PCIE_DOUBLEMAP", "REBOT_DOUBLEMAP", "REBOT_INVALID_HOST"};

  ChimeraTK::setDMapFilePath("./dummies.dmap");
  auto returnedListOfAliases = Utilities::getAliasList();
  ChimeraTK::setDMapFilePath(initialDmapFile);

  int index = 0;
  BOOST_CHECK(returnedListOfAliases.size() == expectedListOfAliases.size());
  for(auto alias : expectedListOfAliases) {
    BOOST_CHECK(alias == returnedListOfAliases.at(index++));
  }
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUserTypeToUserType_Boolean) {
  ChimeraTK::Boolean myBool{true};
  BOOST_TEST(ChimeraTK::userTypeToUserType<std::string>(myBool) == "true");
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(myBool) == 1);

  myBool = false;
  BOOST_TEST(ChimeraTK::userTypeToUserType<std::string>(myBool) == "false");
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(myBool) == 0);

  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("false")) == false);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("False")) == false);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("fAlSe")) == false);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0")) == false);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("")) == false);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("true")) == true);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("TRUE")) == true);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("anyOtherString")) == true);
}

/**********************************************************************************************************************/
