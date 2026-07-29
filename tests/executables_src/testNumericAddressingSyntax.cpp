// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE NumericAddressingSyntaxTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "NumericAddressedRegisterCatalogue.h"

using namespace ChimeraTK;

/**********************************************************************************************************************/

void compareRegister(NumericAddressedRegisterInfo info, RegisterPath const& pathName, uint32_t nElements,
    uint64_t address, uint32_t nBytes, uint64_t bar, uint32_t width, int32_t nFractionalBits, bool signedFlag) {
  BOOST_TEST(info.pathName == pathName);
  BOOST_CHECK_MESSAGE(info.nElements == nElements,
      "nElements mismatch in " << pathName << ": " << info.nElements << " != " << nElements);
  BOOST_CHECK_MESSAGE(
      info.address == address, "address mismatch in " << pathName << ": " << info.address << " != " << address);
  BOOST_CHECK_MESSAGE(info.elementPitchBits / 8 * nElements == nBytes,
      "nBytes mismatch in " << pathName << ": " << info.elementPitchBits / 8 * nElements << " != " << nBytes);
  BOOST_CHECK_MESSAGE(info.bar == bar, "bar mismatch in " << pathName << ": " << info.bar << " != " << bar);
  BOOST_CHECK_MESSAGE(
      info.channels.size() == 1, "channels.size() mismatch in " << pathName << ": " << info.channels.size() << " != 1");
  BOOST_CHECK_MESSAGE(info.channels.front().width == width,
      "width mismatch in " << pathName << ": " << info.channels.front().width << " != " << width);
  BOOST_CHECK_MESSAGE(info.channels.front().nFractionalBits == nFractionalBits,
      "nFractionalBits mismatch in " << pathName << ": " << info.channels.front().nFractionalBits
                                     << " != " << nFractionalBits);
  BOOST_CHECK_MESSAGE(info.channels.front().signedFlag == signedFlag,
      "signedFlag mismatch in " << pathName << ": " << info.channels.front().signedFlag << " != " << signedFlag);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestLegacyScalar) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/0/32");

  compareRegister(info1, "/#/0/32", 1, 32, 4, 0, 32, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestLegacyOneD) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8");

  compareRegister(info1, "/#/5/12*8", 2, 12, 8, 5, 32, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnsigned8) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8u8");

  compareRegister(info1, "/#/5/12*8u8", 8, 12, 8, 5, 8, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestSigned8) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8s8");

  compareRegister(info1, "/#/5/12*8s8", 8, 12, 8, 5, 8, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnsigned16) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8u16");

  compareRegister(info1, "/#/5/12*8u16", 4, 12, 8, 5, 16, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestSigned16) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8s16");

  compareRegister(info1, "/#/5/12*8s16", 4, 12, 8, 5, 16, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnsigned32) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8u32");

  compareRegister(info1, "/#/5/12*8u32", 2, 12, 8, 5, 32, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestSigned32) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8s32");

  compareRegister(info1, "/#/5/12*8s32", 2, 12, 8, 5, 32, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnsigned64) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8u64");

  compareRegister(info1, "/#/5/12*8u64", 1, 12, 8, 5, 64, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestSigned64) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12*8s64");

  compareRegister(info1, "/#/5/12*8s64", 1, 12, 8, 5, 64, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestReverseOrderU32) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12u32*8");

  compareRegister(info1, "/#/5/12u32*8", 2, 12, 8, 5, 32, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestReverseOrderS16) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12s16*8");

  compareRegister(info1, "/#/5/12s16*8", 4, 12, 8, 5, 16, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyU8) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12u8");

  compareRegister(info1, "/#/5/12u8", 1, 12, 1, 5, 8, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyS8) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12s8");

  compareRegister(info1, "/#/5/12s8", 1, 12, 1, 5, 8, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyU16) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12u16");

  compareRegister(info1, "/#/5/12u16", 1, 12, 2, 5, 16, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyS16) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12s16");

  compareRegister(info1, "/#/5/12s16", 1, 12, 2, 5, 16, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyU32) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12u32");

  compareRegister(info1, "/#/5/12u32", 1, 12, 4, 5, 32, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyS32) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12s32");

  compareRegister(info1, "/#/5/12s32", 1, 12, 4, 5, 32, 0, true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyU64) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12u64");

  compareRegister(info1, "/#/5/12u64", 1, 12, 8, 5, 64, 0, false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestOnlyS64) {
  NumericAddressedRegisterCatalogue catalogue;
  auto info1 = catalogue.getBackendRegister("/#/5/12s64");

  compareRegister(info1, "/#/5/12s64", 1, 12, 8, 5, 64, 0, true);
}

/**********************************************************************************************************************/
