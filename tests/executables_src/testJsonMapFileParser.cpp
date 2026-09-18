// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE JsonMapFileParser

#include "Device.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddressedBackend.h"

using namespace ChimeraTK;

#include <boost/pointer_cast.hpp>
#include <boost/test/unit_test.hpp>

#include <limits>
#include <vector>
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
  BOOST_TEST(regs.hasRegister("BSP.VERSION"));
  BOOST_TEST(regs.hasRegister("/BSP/VERSION"));
  BOOST_TEST(regs.hasRegister("DAQ.SIMPLE2D.A"));

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
    BOOST_TEST(reg.isBitRange == false);
    BOOST_TEST(reg.description == "This is an example register");
    BOOST_TEST(reg.engineeringUnit == "mV");
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
    BOOST_TEST(reg.isBitRange == false);
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
    BOOST_TEST(reg.isBitRange == false);
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
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("APP.STATUS.ProbeLimiter");
    BOOST_TEST(reg.pathName == "/APP/STATUS/ProbeLimiter");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_TEST(reg.channels.size() == 1); // for the debug printout if failing
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  {
    auto reg = regs.getBackendRegister("APP.STATUS.ExternalInterlock");
    BOOST_TEST(reg.pathName == "/APP/STATUS/ExternalInterlock");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 1);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  {
    auto reg = regs.getBackendRegister("APP.STATUS.ErrorCounter");
    BOOST_TEST(reg.pathName == "/APP/STATUS/ErrorCounter");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 2);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 3);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  {
    auto reg = regs.getBackendRegister("APP.LARGE_STATUS");
    BOOST_TEST(reg.pathName == "/APP/LARGE_STATUS");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 8 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int64);
    BOOST_TEST(reg.channels[0].width == 64);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("APP.LARGE_STATUS.ProbeLimiter");
    BOOST_TEST(reg.pathName == "/APP/LARGE_STATUS/ProbeLimiter");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 8 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_TEST(reg.channels.size() == 1); // for the debug printout if failing
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int64);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  {
    auto reg = regs.getBackendRegister("APP.LARGE_STATUS.ExternalInterlock");
    BOOST_TEST(reg.pathName == "/APP/LARGE_STATUS/ExternalInterlock");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 8 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 34);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int64);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  {
    auto reg = regs.getBackendRegister("APP.LARGE_STATUS.ErrorCounter");
    BOOST_TEST(reg.pathName == "/APP/LARGE_STATUS/ErrorCounter");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 8 * 8);
    BOOST_TEST(reg.bar == 2);
    BOOST_TEST(reg.address == 0x8004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 2);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int64);
    BOOST_TEST(reg.channels[0].width == 3);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
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
    BOOST_TEST(!reg.doubleBuffer.has_value());
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_TEST(reg.interruptId == std::vector<size_t>({3, 0, 1}), boost::test_tools::per_element());
    BOOST_TEST(reg.doubleBuffer.has_value());
    BOOST_TEST(reg.doubleBuffer->address == 0x40200);
    BOOST_TEST(reg.doubleBuffer->inactiveBufferRegisterPath == "/DAQ.DOUBLE_BUF.INACTIVE_BUF_ID");
    BOOST_TEST(reg.doubleBuffer->enableRegisterPath == "/DAQ.DOUBLE_BUF.ENA");

    BOOST_REQUIRE(reg.channels.size() == 5);

    for(const auto& channel : reg.channels) {
      BOOST_TEST(!channel.selectedBy);
    }

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
    BOOST_TEST(reg.isBitRange == false);
  }
  // Named channel slices of the 2D register DAQ.CTRL: each is a read-only 1D register whose address
  // already contains the channel's byte offset, keeps the full element pitch as stride, and stores a
  // single ChannelInfo with bitOffset 0 and the channel's raw type.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.errorI");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/errorI");
    BOOST_TEST(reg.engineeringUnit == "bits");
    BOOST_TEST(reg.description == "Error signal in I");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_CHECK(reg.getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data) == true);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].nFractionalBits == -2);
    BOOST_TEST(reg.channels[0].signedFlag == true);
    BOOST_TEST(reg.channels[0].getRawType() == ChimeraTK::DataType("int16"));
    // A slice of a non-muxed channel carries no selector
    BOOST_TEST(!reg.channels[0].selectedBy);
    BOOST_TEST(reg.doubleBuffer.has_value());
    BOOST_TEST(reg.doubleBuffer->address == 0x40200);
    BOOST_TEST(reg.doubleBuffer->enableRegisterPath == "/DAQ.DOUBLE_BUF.ENA");
    BOOST_TEST(reg.doubleBuffer->inactiveBufferRegisterPath == "/DAQ.DOUBLE_BUF.INACTIVE_BUF_ID");
    BOOST_TEST(reg.doubleBuffer->index == 0);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.errorQ");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/errorQ");
    BOOST_TEST(reg.engineeringUnit == "bits");
    BOOST_TEST(reg.description == "Error signal in Q");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40000 + 2);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_CHECK(reg.getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data) == true);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].getRawType() == ChimeraTK::DataType("int16"));
    BOOST_TEST(reg.doubleBuffer.has_value());
    BOOST_TEST(reg.doubleBuffer->address == 0x40200 + 2);
    BOOST_TEST(reg.doubleBuffer->enableRegisterPath == "/DAQ.DOUBLE_BUF.ENA");
    BOOST_TEST(reg.doubleBuffer->inactiveBufferRegisterPath == "/DAQ.DOUBLE_BUF.INACTIVE_BUF_ID");
    BOOST_TEST(reg.doubleBuffer->index == 0);
  }
  // A double-buffered named channel slice inherits the parent register's double-buffer configuration. The slice
  // additionally gets its own BUF0/BUF1 buffer-view registers, identical to the parent's but with the channel byte
  // offset folded into both buffer addresses.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.errorI.BUF0");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/errorI/BUF0");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_CHECK(!reg.doubleBuffer.has_value());
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].getRawType() == ChimeraTK::DataType("int16"));
  }
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.errorI.BUF1");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/errorI/BUF1");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40200);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_CHECK(!reg.doubleBuffer.has_value());
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].getRawType() == ChimeraTK::DataType("int16"));
  }
  // A channel with non-zero byte offset folds that offset into both buffer addresses.
  {
    auto reg0 = regs.getBackendRegister("DAQ.CTRL.errorQ.BUF0");
    BOOST_TEST(reg0.address == 0x40000 + 2);
    auto reg1 = regs.getBackendRegister("DAQ.CTRL.errorQ.BUF1");
    BOOST_TEST(reg1.address == 0x40200 + 2);
  }
  // Named channel slice of a non-interrupt 2D register stays read-only and does not advertise
  // wait_for_new_data.
  {
    auto reg = regs.getBackendRegister("DAQ.SIMPLE2D.A");
    BOOST_TEST(reg.pathName == "/DAQ/SIMPLE2D/A");
    BOOST_TEST(reg.bar == 13);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_CHECK(reg.getSupportedAccessModes().has(ChimeraTK::AccessMode::raw) == true);
    BOOST_CHECK(reg.getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data) == false);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].getRawType() == ChimeraTK::DataType("int16"));
    // A slice of a non-muxed SIMPLE2D channel carries no selector
    BOOST_TEST(!reg.channels[0].selectedBy);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.FD");
    BOOST_TEST(reg.pathName == "/DAQ/FD");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x81000);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_TEST(reg.interruptId == std::vector<size_t>({0}), boost::test_tools::per_element());

    // The four channels come from the two former muxed tabs merged into one flat layout, each tagged with
    // its 'selectedBy' register+value. Sorted by byte offset (stable), ties broken by map order:
    // AmplitudeCh0(0), RawCh0(0), PhaseCh0(2), RawCh1(4).
    BOOST_REQUIRE(reg.channels.size() == 4);

    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].nFractionalBits == -2);
    BOOST_TEST(reg.channels[0].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);

    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 32);
    BOOST_TEST(reg.channels[1].nFractionalBits == 0);
    BOOST_TEST(reg.channels[1].signedFlag == true);
    BOOST_REQUIRE(reg.channels[1].selectedBy);
    BOOST_TEST(reg.channels[1].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].selectedBy->val == 1);

    BOOST_TEST(reg.channels[2].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 16);
    BOOST_TEST(reg.channels[2].nFractionalBits == -2);
    BOOST_TEST(reg.channels[2].signedFlag == true);
    BOOST_REQUIRE(reg.channels[2].selectedBy);
    BOOST_TEST(reg.channels[2].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[2].selectedBy->val == 0);

    BOOST_TEST(reg.channels[3].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[3].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[3].width == 32);
    BOOST_TEST(reg.channels[3].nFractionalBits == 0);
    BOOST_TEST(reg.channels[3].signedFlag == true);
    BOOST_REQUIRE(reg.channels[3].selectedBy);
    BOOST_TEST(reg.channels[3].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[3].selectedBy->val == 1);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.DOUBLE_BUF.ENA");
    BOOST_TEST(reg.pathName == "/DAQ/DOUBLE_BUF/ENA");
    BOOST_TEST(reg.nElements == 3);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 1236);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.DOUBLE_BUF.INACTIVE_BUF_ID");
    BOOST_TEST(reg.pathName == "/DAQ/DOUBLE_BUF/INACTIVE_BUF_ID");
    BOOST_TEST(reg.nElements == 3);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 1240);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.FD.BUF0");
    BOOST_TEST(reg.pathName == "/DAQ/FD/BUF0");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x81000);

    BOOST_REQUIRE(reg.channels.size() == 4);

    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].nFractionalBits == -2);
    BOOST_TEST(reg.channels[0].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);

    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 32);
    BOOST_TEST(reg.channels[1].nFractionalBits == 0);
    BOOST_TEST(reg.channels[1].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[1].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].selectedBy->val == 1);

    BOOST_TEST(reg.channels[2].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 16);
    BOOST_TEST(reg.channels[2].nFractionalBits == -2);
    BOOST_TEST(reg.channels[2].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[2].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[2].selectedBy->val == 0);

    BOOST_TEST(reg.channels[3].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[3].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[3].width == 32);
    BOOST_TEST(reg.channels[3].nFractionalBits == 0);
    BOOST_TEST(reg.channels[3].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[3].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[3].selectedBy->val == 1);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.FD.BUF1");
    BOOST_TEST(reg.pathName == "/DAQ/FD/BUF1");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);

    BOOST_REQUIRE(reg.channels.size() == 4);

    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 16);
    BOOST_TEST(reg.channels[0].nFractionalBits == -2);
    BOOST_TEST(reg.channels[0].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);

    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 32);
    BOOST_TEST(reg.channels[1].nFractionalBits == 0);
    BOOST_TEST(reg.channels[1].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[1].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].selectedBy->val == 1);

    BOOST_TEST(reg.channels[2].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 16);
    BOOST_TEST(reg.channels[2].nFractionalBits == -2);
    BOOST_TEST(reg.channels[2].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[2].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[2].selectedBy->val == 0);

    BOOST_TEST(reg.channels[3].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[3].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[3].width == 32);
    BOOST_TEST(reg.channels[3].nFractionalBits == 0);
    BOOST_TEST(reg.channels[3].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[3].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[3].selectedBy->val == 1);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.MUX_SEL");
    BOOST_TEST(reg.pathName == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 0);
    BOOST_TEST(reg.address == 1244);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 2);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.MACRO_PULSE_NUMBER");
    BOOST_TEST(reg.pathName == "/DAQ/MACRO_PULSE_NUMBER");
    BOOST_TEST(reg.nElements == 1);
    BOOST_TEST(reg.elementPitchBits == 4 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x8103C);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
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
    BOOST_TEST(reg.isBitRange == false);
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
    BOOST_TEST(reg.isBitRange == false);
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
    BOOST_TEST(reg.isBitRange == false);
  }

  BOOST_TEST(metas.getNumberOfMetadata() == 6);

  BOOST_TEST(metas.getMetadata("mapfileRevision") == "1.8.3-0-gdeadbeef");
  BOOST_TEST(metas.getMetadata("someRandomEntry") == "some random value");

  BOOST_TEST(metas.getMetadata("![0]") == R"({"INTC":{"options":[],"path":"DAQ","version":1}})");
  BOOST_TEST(metas.getMetadata("![3]") == R"({"INTC":{"options":["MER"],"path":"MY_INTC","version":1}})");
  BOOST_TEST(metas.getMetadata("![3,0]") == R"({"INTC":{"options":[],"path":"MY_INTC.SUB0","version":1}})");
  BOOST_TEST(metas.getMetadata("![3,1]") == R"({"INTC":{"options":["MER"],"path":"MY_INTC.SUB1","version":1}})");

  auto loi = regs.getListOfInterrupts();
  BOOST_TEST(loi.size() == 3);
  BOOST_CHECK(loi.find({0}) != loi.end());
  BOOST_CHECK(loi.find({3, 0, 1}) != loi.end());
  BOOST_CHECK(loi.find({1}) != loi.end());
}

/**********************************************************************************************************************/

// A double-buffered register whose two buffers lie on different BARs is not supported and must fail fast with a
// logic_error at parse time. The fixture is a 2D / named-channel register, directly covering the channel-slice case
// that prompted the check (DoubleBufferingInfo::fill runs before any slices are derived).
BOOST_AUTO_TEST_CASE(TestDoubleBufferDiffBarThrows) {
  ChimeraTK::MapFileParser parser;
  BOOST_CHECK_THROW(parser.parse("simpleJsonFile.diffBar.jmap"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// ChannelInfo::operator== (and !=) must include the selectedByRegister/selectedByValue members. Two channel infos
// that differ only in their selector must compare unequal.
BOOST_AUTO_TEST_CASE(TestChannelInfoEqualitySelectedBy) {
  NumericAddressedRegisterInfo::ChannelInfo a;
  NumericAddressedRegisterInfo::ChannelInfo b;

  // NOTE: ChannelInfo is an aggregate whose scalar members carry no default member initializers, so they must be set
  // explicitly before comparing (default initialization leaves them indeterminate).
  auto initChannelInfo = [](NumericAddressedRegisterInfo::ChannelInfo& c) {
    c.bitOffset = 0;
    c.dataType = NumericAddressedRegisterInfo::Type::FIXED_POINT;
    c.width = 16;
    c.nFractionalBits = 0;
    c.signedFlag = true;
    c.rawType = ChimeraTK::DataType(ChimeraTK::DataType::int16);
    c.selectedBy = std::nullopt;
  };
  initChannelInfo(a);
  initChannelInfo(b);

  // Two identically initialized infos are equal (both unconditional: empty register, default value).
  BOOST_CHECK(a == b);
  BOOST_CHECK(!(a != b));

  BOOST_REQUIRE(!a.selectedBy);
  BOOST_REQUIRE(!b.selectedBy);

  // Differing only in selectedByRegister -> unequal.
  b.selectedBy.emplace(RegisterPath("/DAQ/MUX_SEL"), 0);
  BOOST_CHECK(a != b);
  BOOST_CHECK(!(a == b));

  // Same register again -> equal.
  a.selectedBy.emplace(RegisterPath("/DAQ/MUX_SEL"), 0);
  BOOST_CHECK(a == b);

  // Differing only in selectedByValue -> unequal.
  b.selectedBy->val = 1;
  BOOST_CHECK(a != b);
  BOOST_CHECK(!(a == b));
}

/**********************************************************************************************************************/

// selectedBy with only 'register', no 'value'. Desired semantics (documented): both fields are required, so the
// parser must reject the map with std::logic_error (a missing 'value' must not silently default). NOTE: the current
// lax deserializer does not yet enforce this; this test documents the desired behaviour.
BOOST_AUTO_TEST_CASE(TestSelectedByMissingValue) {
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse("selectedByOnlyRegister.jmap"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// selectedBy with only 'value', no 'register'. Desired semantics (documented): the parser must reject the map
// because both fields are required. NOTE: expected std::logic_error, not guaranteed by the current lax deserializer.
BOOST_AUTO_TEST_CASE(TestSelectedByMissingRegister) {
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse("selectedByOnlyValue.jmap"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// selectedBy on a non-2D (scalar) register is supported: it makes the single register conditional, so the
// register's single channel must carry the selector.
BOOST_AUTO_TEST_CASE(TestSelectedByOnScalar) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("selectedByCases.jmap");
  BOOST_TEST(regs.hasRegister("/SCALAR"));
  auto reg = regs.getBackendRegister("/SCALAR");
  BOOST_TEST(reg.nElements == 1);
  BOOST_REQUIRE(reg.channels.size() == 1);
  BOOST_REQUIRE(reg.channels[0].selectedBy);
  BOOST_TEST(reg.channels[0].selectedBy->regPath == "/MUX");
  BOOST_TEST(reg.channels[0].selectedBy->val == 0);
}

/**********************************************************************************************************************/

// selectedBy with a non-numeric 'value' (e.g. a string) must be rejected with std::logic_error. NOTE: currently a
// nlohmann::json type error surfaces instead; this test documents the desired behaviour.
BOOST_AUTO_TEST_CASE(TestSelectedByBadValue) {
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse("selectedByBadValue.jmap"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// Two channels with the same offset but different selectors, plus one unconditional channel at the same offset.
// The parser must not crash; the ordering/dedup semantics are documented here (both conditional and unconditional
// channels at the same offset constitute an ambiguous mux).
BOOST_AUTO_TEST_CASE(TestSelectedByOffsetCollision) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("selectedByCases.jmap");
  auto reg = regs.getBackendRegister("/COLLISION/FD");
  // All three channels survive (no crash, no merge), order preserved by map order then byte offset.
  BOOST_TEST(reg.channels.size() == 3);
  BOOST_REQUIRE(reg.channels[0].selectedBy);
  BOOST_TEST(reg.channels[0].selectedBy->regPath == "/COLLISION/MUX");
  BOOST_TEST(reg.channels[0].selectedBy->val == 0);
  BOOST_REQUIRE(reg.channels[1].selectedBy);
  BOOST_TEST(reg.channels[1].selectedBy->regPath == "/COLLISION/MUX");
  BOOST_TEST(reg.channels[1].selectedBy->val == 1);
  BOOST_TEST(!reg.channels[2].selectedBy);
}

/**********************************************************************************************************************/

// Alternative selectedBy channels that share a byte offset also share the same register address. In a well-formed
// muxed register the alternatives (different selector values) must resolve to the same address + bit offset.
BOOST_AUTO_TEST_CASE(TestSelectedByAlternativesShareAddress) {
  // Channel-level within a 2D register: /COLLISION/FD has three channels at the same byte offset (Sel0/Sel1/Uncond).
  {
    auto [regs, metas] = ChimeraTK::MapFileParser::parse("selectedByCases.jmap");
    auto reg = regs.getBackendRegister("/COLLISION/FD");
    // Unconditional and muxed alternatives all reside on the same register / address.
    BOOST_REQUIRE(reg.channels.size() == 3);
    BOOST_TEST(reg.address == 0x1000);
    for(const auto& ch : reg.channels) {
      BOOST_TEST(ch.bitOffset == 0);
    }
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);
    BOOST_TEST(reg.channels[1].selectedBy->val == 1);
    BOOST_TEST(reg.channels[0].bitOffset == reg.channels[1].bitOffset);
  }

  // Channel-level within the production /DAQ/FD: AmplitudeCh0(raw) and RawCh0 raw both at bitOffset 0 with different
  // selectors, all within the same register address.
  {
    auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");
    auto reg = regs.getBackendRegister("/DAQ/FD");
    BOOST_REQUIRE(reg.channels.size() == 4);
    BOOST_TEST(reg.address == 0x81000);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);
    BOOST_TEST(reg.channels[1].selectedBy->val == 1);
    BOOST_TEST(reg.channels[0].bitOffset == reg.channels[1].bitOffset);
    // Ambiguity guard: two alternatives at the same offset must not have the same selector value.
    BOOST_TEST(reg.channels[0].selectedBy->val != reg.channels[1].selectedBy->val);
  }
}

/**********************************************************************************************************************/

// Check that the selectBy alternatives share the same addresses
BOOST_AUTO_TEST_CASE(TestSelectedBySingleRegisterInSimpleJsonFile) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");

  BOOST_TEST(regs.hasRegister("/DAQ/SINGLE_MUXED"));
  auto reg0 = regs.getBackendRegister("/DAQ/SINGLE_MUXED");
  BOOST_TEST(reg0.nElements == 1);
  BOOST_REQUIRE(reg0.channels.size() == 1);
  BOOST_REQUIRE(reg0.channels[0].selectedBy);
  BOOST_TEST(reg0.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
  BOOST_TEST(reg0.channels[0].selectedBy->val == 0);

  BOOST_TEST(regs.hasRegister("/DAQ/SINGLE_MUXED_ALT"));
  auto reg1 = regs.getBackendRegister("/DAQ/SINGLE_MUXED_ALT");
  BOOST_TEST(reg1.nElements == 1);
  BOOST_REQUIRE(reg1.channels.size() == 1);
  BOOST_REQUIRE(reg1.channels[0].selectedBy);
  BOOST_TEST(reg1.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
  BOOST_TEST(reg1.channels[0].selectedBy->val == 1);

  // The two muxed alternatives share the same address but differ in their selector value.
  BOOST_TEST(reg0.address == 1246);
  BOOST_TEST(reg1.address == 1246);
  BOOST_TEST(reg0.address == reg1.address);
}

/**********************************************************************************************************************/

// A muxed channel that also has bit-field sub-entries. The named channel slice collides with the
// generated bit-field sub-entries; the parser must not crash and the slice must remain queryable via hasRegister
// (no duplicate registers for the slice path).
BOOST_AUTO_TEST_CASE(TestSelectedByBitFieldSliceCollision) {
  // Parsing must not crash.
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("selectedByCases.jmap");

  // The register and its muxed channel slice are present exactly once.
  BOOST_TEST(regs.hasRegister("/BITFIELD/FD"));
  BOOST_TEST(regs.hasRegister("/BITFIELD/FD/Ch0"));

  // The channel slice persists and carries the selector.
  auto slice = regs.getBackendRegister("/BITFIELD/FD/Ch0");
  BOOST_REQUIRE(slice.channels.size() == 1);
  BOOST_REQUIRE(slice.channels[0].selectedBy);
  BOOST_TEST(slice.channels[0].selectedBy->regPath == "/BITFIELD/MUX");
  BOOST_TEST(slice.channels[0].selectedBy->val == 0);

  // The other muxed channel slice resolves too.
  auto slice1 = regs.getBackendRegister("/BITFIELD/FD/Ch1");
  BOOST_REQUIRE(slice1.channels[0].selectedBy);
  BOOST_TEST(slice1.channels[0].selectedBy->regPath == "/BITFIELD/MUX");
  BOOST_TEST(slice1.channels[0].selectedBy->val == 1);
}

/**********************************************************************************************************************/

// Muxed register combined with doubleBuffering (FD) and interrupt. BUF0/BUF1 slices carry the
// selector and wait_for_new_data is propagated to the muxed slices.
BOOST_AUTO_TEST_CASE(TestSelectedByDoubleBufferAndInterrupt) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");

  // The double-buffer copies mirror the main register's selector info.
  for(const auto* name : {"/DAQ/FD/BUF0", "/DAQ/FD/BUF1"}) {
    auto reg = regs.getBackendRegister(name);
    BOOST_REQUIRE(reg.channels.size() == 4);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);
    BOOST_REQUIRE(reg.channels[1].selectedBy);
    BOOST_TEST(reg.channels[1].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].selectedBy->val == 1);
  }

  // Interrupt propagates wait_for_new_data to the muxed slices.
  auto slice = regs.getBackendRegister("/DAQ/FD/AmplitudeCh0");
  BOOST_CHECK(slice.getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data) == true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
