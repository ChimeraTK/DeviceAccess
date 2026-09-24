// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE JsonMapFileParser

#include "Exception.h"
#include "MapFileParser.h"

using namespace ChimeraTK;

#include <nlohmann/json.hpp>

#include <boost/pointer_cast.hpp>
#include <boost/test/unit_test.hpp>

#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>
using namespace boost::unit_test_framework;

/**********************************************************************************************************************/
/**********************************************************************************************************************/

// Helper used by the selectedBy fault tests below: load simpleJsonFile.jmap as the base map, overwrite the
// selectedBy of the existing COLLISION/FD/Sel0 channel with the supplied (possibly malformed) one, write the result
// to <outFile>, then parse it. This replaces the standalone selectedBy*.jmap fixtures: the fault is injected into an
// existing entry of the full simpleJsonFile map (via nlohmann-json) and the produced file is what gets parsed, so no
// separate fault-only map files need to be kept.
static std::pair<ChimeraTK::NumericAddressedRegisterCatalogue, ChimeraTK::MetadataCatalogue>
    parseInjectedSelectedByFault(const std::string& outFile, const nlohmann::json& selectedBy) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);

  // Overwrite the selectedBy of an existing channel (COLLISION/FD/Sel0) already carrying one, so the fault is
  // injected into an existing entry rather than introducing a new register.
  map["addressSpace"]["COLLISION"]["children"]["FD"]["channels"]["Sel0"]["selectedBy"] = selectedBy;

  std::ofstream(outFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(outFile);
}

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
    BOOST_TEST(regs.hasRegister("/SCALAR"));
    auto reg = regs.getBackendRegister("/SCALAR");
    BOOST_TEST(reg.nElements == 1);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/MUX");
    BOOST_TEST(reg.channels[0].selectedBy->val == 0);
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

// Bit ranges inside the named channels of a 2D register (simpleJsonFile.jmap DAQ.CTRL). A channel with a
// `children` dictionary yields the parent word slice (byte-aligned, unshifted, not a bit range) plus one
// read-only bit-range slice per child. The slices inherit the parent 2D register's address, stride, interrupt
// access and double-buffer settings (including the BUF0/BUF1 buffer views).
BOOST_AUTO_TEST_CASE(TestNamedChannelBitRangeChildren) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");

  // The parent channel slice /DAQ/CTRL/status spans the whole channel word: byte-aligned, unshifted, not a bit
  // range. It inherits nElements (16384) and the 64-byte pitch from the parent 2D register.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.status");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/status");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    // DAQ.CTRL is a DMA register, so its (and the slices') BAR is the DMA pseudo-BAR 13. It is at DMA offset
    // 0x40000 and channel 'status' sits at byte offset 4.
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 32);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == false);
    BOOST_REQUIRE(reg.doubleBuffer);
    BOOST_TEST(reg.doubleBuffer->address == 0x40204);
    BOOST_TEST(regs.hasRegister("DAQ.CTRL.status.BUF0"));
    BOOST_TEST(regs.hasRegister("DAQ.CTRL.status.BUF1"));
  }
  // First bit-field child: bit range at bit offset 0, width 1.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.status.ProbeLimiter");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/status/ProbeLimiter");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
    BOOST_REQUIRE(reg.doubleBuffer);
    BOOST_TEST(reg.doubleBuffer->address == 0x40204);
    BOOST_TEST(regs.hasRegister("DAQ.CTRL.status.ProbeLimiter.BUF0"));
    BOOST_TEST(regs.hasRegister("DAQ.CTRL.status.ProbeLimiter.BUF1"));
  }
  // Second bit-field child at bit offset 1, width 1.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.status.ExternalInterlock");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/status/ExternalInterlock");
    BOOST_TEST(reg.nElements == 16384);
    BOOST_TEST(reg.elementPitchBits == 64 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x40004);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 1);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.isBitRange == true);
  }
  // Third child carries a fixedPoint representation: fractionalBits and isSigned are taken from the child.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.status.ErrorCounter");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/status/ErrorCounter");
    BOOST_TEST(reg.address == 0x40004);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 2);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 3);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  // VectorSum_I has a single child 'I' at bit offset 0, width 18.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.VectorSum_I.I");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/VectorSum_I/I");
    BOOST_TEST(reg.address == 0x40008);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 18);
    BOOST_TEST(reg.isBitRange == true);
  }
  // VectorSum_Q has a single child 'Q' at bit offset 2, width 18.
  {
    auto reg = regs.getBackendRegister("DAQ.CTRL.VectorSum_Q.Q");
    BOOST_TEST(reg.pathName == "/DAQ/CTRL/VectorSum_Q/Q");
    BOOST_TEST(reg.address == 0x4000a);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 2);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 18);
    BOOST_TEST(reg.isBitRange == true);
  }
}

/**********************************************************************************************************************/

// A channel that is itself a single bit range (bitShift/width in its own representation, e.g. channel B of
// bitRangeChannels.jmap) yields a bit-range slice: bitOffset 16, width 8, isBitRange true, read-only.
BOOST_AUTO_TEST_CASE(TestNamedChannelDirectBitRange) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("bitRangeChannels.jmap");

  // Channel B is a direct bit range: bits 16..23 of every sample word.
  {
    auto reg = regs.getBackendRegister("TEST.BR.B");
    BOOST_TEST(reg.pathName == "/TEST/BR/B");
    BOOST_TEST(reg.nElements == 4);
    BOOST_TEST(reg.elementPitchBits == 8 * 8);
    BOOST_TEST(reg.bar == 13);
    BOOST_TEST(reg.address == 0x4004);
    BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 16);
    BOOST_CHECK(reg.channels[0].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 8);
    BOOST_TEST(reg.channels[0].nFractionalBits == 0);
    BOOST_TEST(reg.channels[0].signedFlag == false);
    BOOST_TEST(reg.isBitRange == true);
  }
  // Contrast with channel A of the same register: exposing the full word it is not a bit range.
  {
    auto reg = regs.getBackendRegister("TEST.BR.A");
    BOOST_TEST(reg.address == 0x4000);
    BOOST_TEST(reg.isBitRange == false);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[0].width == 32);
  }
  // The bit-field child slices of channel A remain bit ranges.
  {
    auto reg = regs.getBackendRegister("TEST.BR.A.Hi");
    BOOST_TEST(reg.channels[0].bitOffset == 8);
    BOOST_TEST(reg.isBitRange == true);
  }
}

/**********************************************************************************************************************/

// A channel child that is not a proper bit range of the channel word must be ignored while the supported sibling
// children are still created. 'Good' (bitShift 0, width 8, within a 32-bit word) is a valid bit range; 'Whole'
// spans the entire channel word (bitShift 0, width 32) and 'Overflow' extends past it (bitShift 28, width 8).
BOOST_AUTO_TEST_CASE(TestNamedChannelBitRangeIgnoredChildren) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("bitRangeChildCases.jmap");

  // The valid child slice is present.
  {
    auto reg = regs.getBackendRegister("CHILD.A.Good");
    BOOST_TEST(reg.pathName == "/CHILD/A/Good");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_CHECK(reg.channels[0].rawType == DataType::int32);
    BOOST_TEST(reg.channels[0].width == 8);
    BOOST_TEST(reg.isBitRange == true);
  }
  // The unsupported children must not be present.
  BOOST_CHECK(!regs.hasRegister("CHILD.A.Whole"));
  BOOST_CHECK(!regs.hasRegister("CHILD.A.Overflow"));
  // The parent channel slice still exists.
  BOOST_CHECK(regs.hasRegister("CHILD.A"));
}

/**********************************************************************************************************************/

// A channel literally named 'BUF0' in a double-buffered 2D register collides with the parent's auto-generated BUF0
// buffer view. This is a map authoring error and must fail fast with a logic_error at parse time. The colliding
// channel is injected into a copy of simpleJsonFile.jmap (the double-buffered DAQ.CTRL register) with nlohmann::json,
// written to a temporary file, and that file is parsed.
BOOST_AUTO_TEST_CASE(TestNamedChannelBitRangeBuf0CollisionThrows) {
  // Load the existing jmap file and inject a channel named 'BUF0' into the double-buffered DAQ.CTRL register.
  std::ifstream in("simpleJsonFile.jmap");
  nlohmann::json doc;
  in >> doc;
  doc["addressSpace"]["DAQ"]["children"]["CTRL"]["channels"]["BUF0"] = {{"bytesPerElement", 4}, {"offset", 6},
      {"representation", {{"type", "fixedPoint"}, {"width", 16}, {"fractionalBits", 0}}}};
  std::string tmpFile = "bitRangeBuf0Collision_" + std::to_string(getpid()) + ".jmap";
  std::ofstream out(tmpFile);
  out << doc.dump();
  out.close();

  ChimeraTK::MapFileParser parser;
  BOOST_CHECK_THROW(parser.parse(tmpFile), ChimeraTK::logic_error);

  std::remove(tmpFile.c_str());
}

// A bit-field child literally named 'BUF0' of a double-buffered channel collides with the parent channel slice's
// auto-generated BUF0 buffer view. This is a map authoring error and must fail fast with a logic_error at parse
// time, rather than silently skipping the child slice. The colliding child is injected into a copy of
// simpleJsonFile.jmap (whose DAQ.CTRL 'status' channel already carries children) with nlohmann::json, written to a
// temporary file, and that file is parsed.
BOOST_AUTO_TEST_CASE(TestNamedChannelBitRangeChildBuf0CollisionThrows) {
  // Load the existing jmap file and add a child named 'BUF0' to the 'status' channel of DAQ.CTRL.
  std::ifstream in("simpleJsonFile.jmap");
  nlohmann::json doc;
  in >> doc;
  doc["addressSpace"]["DAQ"]["children"]["CTRL"]["channels"]["status"]["children"]["BUF0"] = {
      {"representation", {{"bitShift", 8}, {"width", 8}}}};
  std::string tmpFile = "bitRangeChildBuf0Collision_" + std::to_string(getpid()) + ".jmap";
  std::ofstream out(tmpFile);
  out << doc.dump();
  out.close();

  ChimeraTK::MapFileParser parser;
  BOOST_CHECK_THROW(parser.parse(tmpFile), ChimeraTK::logic_error);

  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A bit-field child slice of a muxed channel inherits the channel's selectedBy condition (fixture
// selectedByCases.jmap BITFIELD/FD/Ch0, which combines a per-channel 'selectedBy' with a 'children' dictionary).
// Every child slice carries the same register+value selector as the parent channel slice.
BOOST_AUTO_TEST_CASE(TestNamedChannelBitRangeMuxedChildSelectedBy) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");

  // The muxed parent channel slice inherits the selectedBy condition.
  {
    auto reg = regs.getBackendRegister("DAQ.MUXED_WITH_STATUS.StatusCh0");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 2);
    BOOST_TEST(reg.isBitRange == false);
  }
  // Each bit-field child slice of the muxed channel inherits the channel's selectedBy condition.
  {
    auto reg = regs.getBackendRegister("DAQ.MUXED_WITH_STATUS.StatusCh0.ProbeLimiter");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.isBitRange == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 2);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.MUXED_WITH_STATUS.StatusCh0.ErrorCounter");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 2);
    BOOST_TEST(reg.channels[0].width == 3);
    BOOST_TEST(reg.isBitRange == true);
    BOOST_REQUIRE(reg.channels[0].selectedBy);
    BOOST_TEST(reg.channels[0].selectedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].selectedBy->val == 2);
  }
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
  nlohmann::json sel{{"register", "COLLISION.MUX"}}; // missing 'value'
  BOOST_CHECK_THROW(parseInjectedSelectedByFault("selectedByMissingValue.jmap", sel), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// selectedBy with only 'value', no 'register'. Desired semantics (documented): the parser must reject the map
// because both fields are required. NOTE: expected std::logic_error, not guaranteed by the current lax deserializer.
BOOST_AUTO_TEST_CASE(TestSelectedByMissingRegister) {
  nlohmann::json sel{{"value", 0}}; // missing 'register'
  BOOST_CHECK_THROW(parseInjectedSelectedByFault("selectedByMissingRegister.jmap", sel), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// S1: 'selectedBy' referencing a selector register that does not exist in the catalogue must be rejected.
BOOST_AUTO_TEST_CASE(TestSelectedBySelectorNotFound) {
  nlohmann::json map;
  map["mapFormatVersion"] = "0.0.1";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  // APP.DATA gates on a selector register 'APP.NONEXISTENT' that is never declared.
  map["addressSpace"]["APP"]["children"]["DATA"] = {
      {"numberOfElements", 1},
      {"access", "RO"},
      {"selectedBy", {{"register", "APP.NONEXISTENT"}, {"value", 1}}},
      {"address", {{"channel", 0}, {"offset", 4}}},
      {"representation", {{"width", 32}}}};
  std::string tmpFile = "selectedBySelectorNotFound_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
}

// S2: a selector register that is itself conditionally enabled by a 'selectedBy' of its own must be rejected: a
// selector drives the gate of another register and must therefore be readable unconditionally.
BOOST_AUTO_TEST_CASE(TestSelectedBySelectorItselfGated) {
  nlohmann::json map;
  map["mapFormatVersion"] = "0.0.1";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  // APP.SEL is itself gated by APP.MASTER_SEL, and APP.DATA gates on APP.SEL. The selector APP.SEL must not be gated.
  map["addressSpace"]["APP"]["children"]["SEL"] = {
      {"numberOfElements", 1},
      {"access", "RO"},
      {"selectedBy", {{"register", "APP.MASTER_SEL"}, {"value", 1}}},
      {"address", {{"channel", 0}, {"offset", 0}}},
      {"representation", {{"width", 32}}}};
  map["addressSpace"]["APP"]["children"]["MASTER_SEL"] = {
      {"numberOfElements", 1},
      {"access", "RO"},
      {"address", {{"channel", 0}, {"offset", 8}}},
      {"representation", {{"width", 32}}}};
  map["addressSpace"]["APP"]["children"]["DATA"] = {
      {"numberOfElements", 1},
      {"access", "RO"},
      {"selectedBy", {{"register", "APP.SEL"}, {"value", 1}}},
      {"address", {{"channel", 0}, {"offset", 4}}},
      {"representation", {{"width", 32}}}};
  std::string tmpFile = "selectedBySelectorItselfGated_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// selectedBy on a non-2D (scalar) register is supported: it makes the single register conditional, so the
// register's single channel must carry the selector.
BOOST_AUTO_TEST_CASE(TestSelectedByOnScalar) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");
}

/**********************************************************************************************************************/

// A 'selectedBy' declared on a module or parent register is inherited by all descendant registers/channels that lack
// their own declaration; the nearest ancestor's declaration wins for a node that has its own.
BOOST_AUTO_TEST_CASE(TestSelectedByInheritance) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("selectedByInheritance.jmap");

  // Helpers: assert the single channel of a scalar/1D register carries the given selector (or none).
  auto checkRegSelectedBy = [](const NumericAddressedRegisterInfo& reg, const std::string& expectedReg,
                                std::optional<int64_t> expectedVal) {
    BOOST_REQUIRE(reg.channels.size() == 1);
    const auto& sb = reg.channels[0].selectedBy;
    if(expectedVal.has_value()) {
      BOOST_REQUIRE(sb.has_value());
      BOOST_TEST(sb->regPath == expectedReg);
      BOOST_TEST(sb->val == *expectedVal);
    }
    else {
      BOOST_CHECK(!sb.has_value());
    }
  };
  // Helpers: assert each channel of a 2D register, in natural (byte-offset) order.
  auto check2DChannelsSelectedBy = [](const NumericAddressedRegisterInfo& reg,
                                       std::initializer_list<std::pair<std::string, int64_t>> expected) {
    BOOST_REQUIRE(reg.channels.size() == expected.size());
    size_t i = 0;
    for(const auto& [expectedReg, expectedVal] : expected) {
      const auto& sb = reg.channels[i].selectedBy;
      BOOST_REQUIRE(sb.has_value());
      BOOST_TEST(sb->regPath == expectedReg);
      BOOST_TEST(sb->val == expectedVal);
      ++i;
    }
  };

  // Module inheritance. REG_A inherits the module default; REG_B keeps its own (override).
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/REG_A"), "/APP/OUTPUT_SELECT", 3);
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/REG_B"), "/CTRL/MODE", 9);

  // Nested inheritance (module -> submodule -> leaf). SUB carries no selectedBy, so LEAF inherits from INHERIT.
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/SUB/LEAF"), "/APP/OUTPUT_SELECT", 3);

  // 2D module default. All channels of a contained 2D register without own declarations inherit the module default.
  check2DChannelsSelectedBy(
      regs.getBackendRegister("/INHERIT/CHANA"), {{"/APP/OUTPUT_SELECT", 3}, {"/APP/OUTPUT_SELECT", 3}});

  // Channel override. D1 has its own per-channel selectedBy; D0 inherits the module default.
  check2DChannelsSelectedBy(
      regs.getBackendRegister("/INHERIT/CHANB"), {{"/APP/OUTPUT_SELECT", 3}, {"/CTRL/CHAN_SEL", 7}});

  // Register-module dual role. Its selectedBy applies to its own register AND to its descendant.
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/REG2D_OWN"), "/CTRL/DUAL", 5);
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/REG2D_OWN/CHILD"), "/CTRL/DUAL", 5);

  // Register-level selectedBy directly on a 2D register (no module) is valid and becomes the default for all channels.
  check2DChannelsSelectedBy(regs.getBackendRegister("/REG2D_TOP"), {{"/APP/TOP_SEL", 11}, {"/APP/TOP_SEL", 11}});

  // A leaf with no ancestor carrying selectedBy stays unconditional (no inheritance).
  checkRegSelectedBy(regs.getBackendRegister("/LEAF_TOP"), "", std::nullopt);

  // The 1D channel slices of an inherited 2D register also carry the effective selector.
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/CHANA/C0"), "/APP/OUTPUT_SELECT", 3);
  checkRegSelectedBy(regs.getBackendRegister("/INHERIT/CHANA/C1"), "/APP/OUTPUT_SELECT", 3);
  checkRegSelectedBy(regs.getBackendRegister("/REG2D_TOP/E0"), "/APP/TOP_SEL", 11);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
