// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ChimeraTK/Device.h>
#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE JsonMapFileParser

#include "Exception.h"
#include "MapFileParser.h"

using namespace ChimeraTK;

#include <boost/test/unit_test.hpp>
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

  {
    auto reg = regs.getBackendRegister("/SomeTopLevelRegister");
    BOOST_TEST(reg.pathName == "/SomeTopLevelRegister");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 32);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_WRITE);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 8);
    BOOST_TEST(reg.channels[0].signedFlag == true);
  }
  {
    auto reg = regs.getBackendRegister("BSP.VERSION");
    BOOST_TEST(reg.pathName == "/BSP/VERSION");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 4);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("BSP");
    BOOST_TEST(reg.pathName == "/BSP");
    BOOST_TEST(reg.nElements == 19201);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 0);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_WRITE);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("APP.STATUS");
    BOOST_TEST(reg.pathName == "/APP/STATUS");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("APP.SomeTable");
    BOOST_TEST(reg.pathName == "/APP/SomeTable");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 2 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 2048);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::WRITE_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 14);
    BOOST_TEST(reg.channels[0].nFractionalBits == 10);
    BOOST_TEST(reg.channels[0].signedFlag == true);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x80000000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_TEST(reg.interruptId == std::vector<size_t>({3, 0, 1}), boost::test_tools::per_element());

    BOOST_REQUIRE(reg.channels.size() == 5);

    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].nFractionalBits == -2);
    BOOST_TEST(reg.channels[0].signedFlag == true);

    BOOST_TEST(reg.channels[1].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 16);
    BOOST_TEST(reg.channels[1].nFractionalBits == -2);
    BOOST_TEST(reg.channels[1].signedFlag == true);

    BOOST_TEST(reg.channels[2].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 32);
    BOOST_TEST(reg.channels[2].nFractionalBits == 0);
    BOOST_TEST(reg.channels[2].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.FD");
    BOOST_TEST(reg.pathName == "/DAQ/FD");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x81000000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_TEST(reg.interruptId == std::vector<size_t>({0}), boost::test_tools::per_element());

    BOOST_REQUIRE(reg.channels.size() == 2);

    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].nFractionalBits == -2);
    BOOST_TEST(reg.channels[0].signedFlag == true);

    BOOST_TEST(reg.channels[1].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 16);
    BOOST_TEST(reg.channels[1].nFractionalBits == -2);
    BOOST_TEST(reg.channels[1].signedFlag == true);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.DOUBLE_BUF.ENA");
    BOOST_TEST(reg.pathName == "/DAQ/DOUBLE_BUF/ENA");
    BOOST_TEST(reg.nElements == 3);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 1234);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.DOUBLE_BUF.INACTIVE_BUF_ID");
    BOOST_TEST(reg.pathName == "/DAQ/DOUBLE_BUF/INACTIVE_BUF_ID");
    BOOST_TEST(reg.nElements == 3);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 1238);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.MUX_SEL");
    BOOST_TEST(reg.pathName == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 1242);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 2);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.MACRO_PULSE_NUMBER");
    BOOST_TEST(reg.pathName == "/DAQ/MACRO_PULSE_NUMBER");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x8100003C);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("BSP.SOME_INFO");
    BOOST_TEST(reg.pathName == "/BSP/SOME_INFO");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 40 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 8);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::ASCII);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }
  {
    auto reg = regs.getBackendRegister("APP.SomeFloat");
    BOOST_TEST(reg.pathName == "/APP/SomeFloat");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 4096);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::IEEE754);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == true);
  }
  {
    auto reg = regs.getBackendRegister("BSP.VOID_INTERRUPT_0");
    BOOST_TEST(reg.pathName == "/BSP/VOID_INTERRUPT_0");
    BOOST_TEST(reg.nElements == 0);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::VOID);
    BOOST_TEST(reg.channels[0].width == 0);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
  }

  BOOST_TEST(metas.getNumberOfMetadata() == 6);

  BOOST_TEST(metas.getMetadata("mapfileRevision") == "1.8.3-0-gdeadbeef");
  BOOST_TEST(metas.getMetadata("someRandomEntry") == "some random value");

  BOOST_TEST(metas.getMetadata("![0]") == R"({"INTC":{"options":[],"path":"DAQ","version":1}})");
  BOOST_TEST(metas.getMetadata("![3]") == R"({"INTC":{"options":["MER"],"path":"MY_INTC","version":1}})");
  BOOST_TEST(metas.getMetadata("![3,0]") == R"({"INTC":{"options":[],"path":"MY_INTC.SUB0","version":1}})");
  BOOST_TEST(metas.getMetadata("![3,1]") == R"({"INTC":{"options":["MER"],"path":"MY_INTC.SUB1","version":1}})");

  auto loi = regs.getListOfInterrupts();
  BOOST_TEST(loi.size() == 2);
  BOOST_CHECK(loi.find({0}) != loi.end());
  BOOST_CHECK(loi.find({3, 0, 1}) != loi.end());
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestInterruptIntegration) {
  ChimeraTK::Device dev("(dummy?map=simpleJsonFile.jmap)");

  dev.open();

  auto int0 = dev.getVoidRegisterAccessor("/BSP/VOID_INTERRUPT_0", {ChimeraTK::AccessMode::wait_for_new_data});
  auto int301 = dev.getVoidRegisterAccessor("/BSP/VOID_INTERRUPT_3_0_1", {ChimeraTK::AccessMode::wait_for_new_data});
  dev.activateAsyncRead();
  BOOST_TEST(int0.readNonBlocking() == true);
  BOOST_TEST(int301.readNonBlocking() == true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
