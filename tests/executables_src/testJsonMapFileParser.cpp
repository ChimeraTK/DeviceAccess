// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE JsonMapFileParser
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Exception.h"
#include "MapFileParser.h"

using namespace ChimeraTK;
using namespace boost::unit_test_framework;

BOOST_AUTO_TEST_SUITE(JsonMapFileParserTestSuite)

/**********************************************************************************************************************/
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestFileDoesNotExist) {
  ChimeraTK::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("NonexistentFile.jmap"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGoodMapFileParse) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");

  BOOST_TEST(regs.hasRegister("/SomeTopLevelRegister"));
  auto reg = regs.getBackendRegister("/SomeTopLevelRegister");
  BOOST_TEST(reg.pathName == "/SomeTopLevelRegister");
  BOOST_TEST(reg.nElements == 1);
  // BOOST_TEST(reg.elementPitchBits == 1);
  BOOST_TEST(reg.bar == 2);
  BOOST_TEST(reg.address == 1234);
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
  BOOST_REQUIRE(reg.channels.size() == 1);
  BOOST_TEST(reg.channels[0].bitOffset == 0);
  BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
  BOOST_TEST(reg.channels[0].width == 32);
  BOOST_TEST(reg.channels[0].nFractionalBits == 0);
  BOOST_TEST(reg.channels[0].signedFlag == true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
