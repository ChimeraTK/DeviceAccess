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

#include <cstdint>
#include <limits>

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
  std::string testFilePath = "./dummies.dmap";
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
  // BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("00")) == false); //FIXME this should pass
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("")) == false);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("true")) == true);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("TRUE")) == true);
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("anyOtherString")) == true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUserTypeToUserType_HexString) {
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint64_t>(std::string("banana"))); // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0xG")));    // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0x0")) == false); // min, mid
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0X1")) == true);  // max, mid, big X
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0x00BA0000F0cacc1a")) ==
      true); // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0xDung"))); // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0x")));     // empty test
  // FIXME this should pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0x000")) == false);  // odd extra zeros

  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("0x66"))) == 0x66); // mid+,
  BOOST_TEST(
      static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("0X7F"))) == INT8_MAX); // max, big X
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("0x00BA0000F0cacc1a"))) ==
      INT8_MAX);                                                                      // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int8_t>(std::string("0xDung"))); // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int8_t>(std::string("0x")));     // empty test

  BOOST_TEST(static_cast<uint32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0x66"))) == 0x66); // mid+
  BOOST_TEST(
      static_cast<uint32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0XFF"))) == UINT8_MAX); // max, big X
  BOOST_TEST(static_cast<uint32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0x0"))) == 0);   // min
  BOOST_TEST(static_cast<uint32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0x00BA0000F0cacc1a"))) ==
      UINT8_MAX);                                                                      // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0xDung"))); // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0x")));     // empty test

  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("0x6666")) == 0x6666);    // 0x6666 = 26214 mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("0X7FFF")) == INT16_MAX); // max, big X
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<int16_t>(std::string("0x00BA0000F0cacc1a")) == INT16_MAX); // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int16_t>(std::string("0xDung")));         // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int16_t>(std::string("0x")));             // empty test

  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("0x6666")) == 0x6666);     // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("0XFFFF")) == UINT16_MAX); // max, big X
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("0x0")) == 0);             // min
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<uint16_t>(std::string("0x00BA0000F0cacc1a")) == UINT16_MAX); // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint16_t>(std::string("0xDung")));          // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint16_t>(std::string("0x")));              // empty test

  BOOST_TEST(
      ChimeraTK::userTypeToUserType<int32_t>(std::string("0x66666666")) == 0x6666'6666); // 0x6666'6666=1717986918 mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("0X7FFFFFFF")) == INT32_MAX); // max, big X
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<int32_t>(std::string("0x00BA0000F0cacc1a")) == INT32_MAX); // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int32_t>(std::string("0xDung")));         // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int32_t>(std::string("0x")));             // empty test

  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("0x66666666")) == 0x6666'6666); // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("0XFFFFFFFF")) == UINT32_MAX);  // max, big X
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("0x0")) == 0);                  // min
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<uint32_t>(std::string("0x00BA0000F0cacc1a")) == UINT32_MAX); // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint32_t>(std::string("0xDung")));          // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint32_t>(std::string("0x")));              // empty test

  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("0x6666666666666666")) ==
      0x6666'6666'6666'6666); // = 7378697629483820646 mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("0X7FFFFFFFFFFFFFFF")) == INT64_MAX); // max, big X
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<int64_t>(std::string("0x100BA0000F0cacc1a")) == INT64_MAX); // overflow, mixed case
  // The previous two lines are likely to run out of room and report 0
  BOOST_CHECK_NO_THROW(
      ChimeraTK::userTypeToUserType<int64_t>(std::string("0xdung"))); // invalid, try lower case for a change.
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int64_t>(std::string("0x"))); // empty test

  BOOST_TEST(
      ChimeraTK::userTypeToUserType<uint64_t>(std::string("0xC0CAC01AADD511FE")) == 0xC0CA'C01A'ADD5'11FE); // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0XFFFFFFFFFFFFFFFF")) == UINT64_MAX); // max, big X
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0x0")) == 0);                         // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0xFFFFFFFFFF0cacc1a")) ==
      UINT64_MAX);                                                                      // overflow, mixed case
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0xDung"))); // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0x")));     // empty test

  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<float>(std::string("0x66666666")), 1.71799e9, 1.71799e5);
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<float>(std::string("0xDung"))); // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<float>(std::string("0x")));     // empty test
                                                                                     //
  BOOST_CHECK_CLOSE(
      ChimeraTK::userTypeToUserType<double>(std::string("0x6666666666666666")), 7.3786976e18, 7.3786976e14); // mid+
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<double>(std::string("0xDung")));                        // invalid
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<double>(std::string("0x"))); // empty test

  BOOST_TEST(ChimeraTK::userTypeToUserType<std::string>(std::string("0xDung")) == "0xDung");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUserTypeToUserType_DecString) {
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("0")) == false); // mid+, min
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("1")) == true);  // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("2")) == true);  // overflow
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<ChimeraTK::Boolean>(std::string("-7378697629483820646")) == true); // underflow
  // invalid case is done in testUserTypeToUserType_Boolean

  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("102"))) == 102);       // mid+
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("-102"))) == -102);     // mid-
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("127"))) == INT8_MAX);  // max
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("-128"))) == INT8_MIN); // min
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("300"))) == INT8_MAX);  // overflow
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("-300"))) == INT8_MIN); // underflow
  // FIXME make this pass
  // BOOST_TEST(
  //    static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("73786976294838206460"))) == INT8_MAX);
  // overflow

  // FIXME make this pass
  // BOOST_TEST(
  //    static_cast<int32_t>(ChimeraTK::userTypeToUserType<int8_t>(std::string("-73786976294838206460"))) == INT8_MIN);
  //  underflow

  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int8_t>(std::string("banana"))); // invalid

  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("102"))) == 102);       // mid+
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("255"))) == UINT8_MAX); // max
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("0"))) == 0);           // min
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("300"))) == UINT8_MAX); // overflow
  BOOST_TEST(static_cast<int32_t>(ChimeraTK::userTypeToUserType<uint8_t>(std::string("-5"))) == 0); // underflow pass
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint8_t>(std::string("banana")));              // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("26214")) == 26214);                     // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("-26214")) == -26214);                   // mid-
  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("32767")) == INT16_MAX);                 // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("-32768")) == INT16_MIN);                // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("73786976294838206460")) == INT16_MAX);  // overflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<int16_t>(std::string("-73786976294838206460")) == INT16_MIN); // underflow
  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<int16_t>(std::string("banana")));                   // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("26214")) == 26214);                     // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("65535")) == UINT16_MAX);                // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("0")) == 0);                             // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("73786976294838206460")) == UINT16_MAX); // overflow
  // FIXME make this pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("-73786976294838206460")) == 0); // underflow
  // FIXME make this pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<uint16_t>(std::string("-5")) == 0); // underflow fail -5 = 65531.

  BOOST_CHECK_NO_THROW(ChimeraTK::userTypeToUserType<uint16_t>(std::string("banana"))); // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("1717986918")) == 1717986918);           // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("-1717986918")) == -1717986918);         // mid-
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("2147483647")) == INT32_MAX);            // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("-2147483648")) == INT32_MIN);           // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("73786976294838206460")) == INT32_MAX);  // overflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("-73786976294838206460")) == INT32_MIN); // underflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<int32_t>(std::string("banana")) == 0);                        // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("1717986918")) == 1717986918);           // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("4294967295")) == UINT32_MAX);           // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("0")) == 0);                             // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("73786976294838206460")) == UINT32_MAX); // overflow
  // FIXME make this pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("-73786976294838206460")) == 0); // underflow
  // FIXME make this pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("-5")) == 0);                            // underflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint32_t>(std::string("banana")) == 0); // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("7378697629483820646")) == 7378697629483820646); // mid+
  BOOST_TEST(
      ChimeraTK::userTypeToUserType<int64_t>(std::string("-7378697629483820646")) == -7378697629483820646); // mid-
  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("9223372036854775807")) == INT64_MAX);      // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("-9223372036854775808")) == INT64_MIN);     // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("9223372036854775810")) == INT64_MAX);      // overflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("-9223372036854775810")) == INT64_MIN);     // underflow
  // The previous two lines are likely to run out of room and report 0
  BOOST_TEST(ChimeraTK::userTypeToUserType<int64_t>(std::string("banana")) == 0); // invalid

  BOOST_TEST(
      ChimeraTK::userTypeToUserType<uint64_t>(std::string("7378697629483820646")) == 7378697629483820646); // mid+
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("18446744073709551615")) == UINT64_MAX);  // max
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("0")) == 0);                              // min
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("18446744073709551625")) == UINT64_MAX);  // overflow
  // FIXME make this pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("-18446744073709551625")) == 0);        // underflow
  // FIXME make this pass
  // BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("-5")) == 0);                           // underflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<uint64_t>(std::string("banana")) == 0); // invalid

  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<float>(std::string("3.14159")), 3.14159, 3.14159 / 1000.0); // mid+
  BOOST_CHECK_CLOSE(
      ChimeraTK::userTypeToUserType<float>(std::string("-3.14159e3")), -3.14159e3, 3.14159e3 / 1000.0); // mid-
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<float>(std::string("3.40282e38")), std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max() / 1000.); // max
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<float>(std::string("-3.40282e38")),
      (-std::numeric_limits<float>::max()), std::numeric_limits<float>::max() / 1000.); // min
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<float>(std::string("5.40282e39")), std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max() / 1000.); // overflow
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<float>(std::string("-5.40282e39")),
      (-std::numeric_limits<float>::max()), std::numeric_limits<float>::max() / 1000.); // underflow
  BOOST_TEST(ChimeraTK::userTypeToUserType<float>(std::string("banana")) == 0.f);       // invalid
  BOOST_CHECK_CLOSE(
      ChimeraTK::userTypeToUserType<float>(std::string("5")), 5.0f, 5.0 / 1000.); // float-int conversion good

  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<double>(std::string("2.718281828459")), 2.718281828459,
      2.718281828459 / 1000.0); // mid+
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<double>(std::string("-2.718281828459e3")), -2.718281828459e3,
      2.718281828459e3 / 1000.0); // mid-
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<double>(std::string("1.7976931e+308")),
      std::numeric_limits<double>::max(), std::numeric_limits<double>::max() / 1000.); // max
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<double>(std::string("-1.7976931e+308")),
      (-std::numeric_limits<double>::max()), std::numeric_limits<double>::max() / 1000.); // min
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<double>(std::string("1.8976931e+309")),
      std::numeric_limits<double>::max(), std::numeric_limits<double>::max() / 1000.); // overflow
  BOOST_CHECK_CLOSE(ChimeraTK::userTypeToUserType<double>(std::string("-1.8976931e+309")),
      (-std::numeric_limits<double>::max()), std::numeric_limits<double>::max() / 1000.); // underflow
  BOOST_CHECK_CLOSE(
      ChimeraTK::userTypeToUserType<double>(std::string("5")), 5.0, 5.0 / 1000.);  // double-int conversion good
  BOOST_TEST(ChimeraTK::userTypeToUserType<double>(std::string("banana")) == 0.0); // invalid

  BOOST_TEST(ChimeraTK::userTypeToUserType<std::string>(std::string("Any\r\nthing")) == "Any\r\nthing");
}
