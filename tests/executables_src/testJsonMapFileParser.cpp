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

// Helper used by the map format version tests below: load simpleJsonFile.jmap as the base map, set the top-level
// mapFormatVersion entry to the supplied value (or remove it when the value is null), write the result to <outFile>,
// then parse it. As with the selectedBy fault helper above, this avoids keeping a set of near-identical variant
// fixtures: the version under test is injected into an existing full map and the produced file is what gets parsed.
static std::pair<ChimeraTK::NumericAddressedRegisterCatalogue, ChimeraTK::MetadataCatalogue> parseWithMapFormatVersion(
    const std::string& outFile, const nlohmann::json& version) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);

  if(version.is_null()) {
    map.erase("mapFormatVersion");
  }
  else {
    map["mapFormatVersion"] = version;
  }

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

  BOOST_TEST(metas.getMetadata("![0]") == R"({"INTC":{"path":"DAQ","version":1}})");
  BOOST_TEST(metas.getMetadata("![3]") == R"({"INTC":{"options":["MER"],"path":"MY_INTC","version":1}})");
  BOOST_TEST(metas.getMetadata("![3,0]") == R"({"INTC":{"options":[],"path":"MY_INTC.SUB0","version":1}})");
  BOOST_TEST(metas.getMetadata("![3,1]") == R"({"INTC":{"options":["MER"],"path":"MY_INTC.SUB1","version":1}})");
  // The map derives interrupt ids for the ISR bit fields of every INTC handler: DAQ/ISR yields [0, 0], [0, 1] and
  // [0, 2], the nested MY_INTC.SUB1/ISR yields [3, 1, 0] and [3, 1, 1]. The dedicated cases below assert the derived
  // registers themselves; here only the full list of interrupts is checked, which no dedicated case covers.
  auto loi = regs.getListOfInterrupts();
  BOOST_TEST(loi.size() == 8);
  BOOST_CHECK(loi.find({0}) != loi.end());
  BOOST_CHECK(loi.find({0, 0}) != loi.end());
  BOOST_CHECK(loi.find({0, 1}) != loi.end());
  BOOST_CHECK(loi.find({0, 2}) != loi.end());
  BOOST_CHECK(loi.find({3, 0, 1}) != loi.end());
  BOOST_CHECK(loi.find({3, 1, 0}) != loi.end());
  BOOST_CHECK(loi.find({3, 1, 1}) != loi.end());
  BOOST_CHECK(loi.find({1}) != loi.end());
}

/**********************************************************************************************************************/

// The 1-bit children of an INTC handler's <path>/ISR register derive interrupt registers carrying the handler id
// followed by the child's bit position. The child keeps its path, address and bit position; only its access changes to
// interrupt.
BOOST_AUTO_TEST_CASE(TestDerivedInterruptRegistersFromIsr) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");
  (void)metas;

  auto reg = regs.getBackendRegister("/DAQ/ISR/DAQ_READY");
  BOOST_TEST(reg.pathName == "/DAQ/ISR/DAQ_READY");
  BOOST_TEST(reg.nElements == 1);
  BOOST_TEST(reg.elementPitchBits == 4 * 8);
  BOOST_TEST(reg.bar == 5);
  BOOST_TEST(reg.address == 8);
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{0, 0}));
  BOOST_REQUIRE(reg.channels.size() == 1);
  BOOST_TEST(reg.channels[0].bitOffset == 0);
  BOOST_TEST(reg.channels[0].width == 1);
  BOOST_TEST(reg.isBitRange == true);
  BOOST_TEST(regs.hasRegister("/DAQ/ISR/DAQ_READY"));

  // The bit position is the child's own bit field offset within the ISR word.
  auto reg2 = regs.getBackendRegister("/DAQ/ISR/THIRD_INTERRUPT");
  BOOST_CHECK(reg2.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg2.interruptId == std::vector<size_t>{0, 2}));
  BOOST_REQUIRE(reg2.channels.size() == 1);
  BOOST_TEST(reg2.channels[0].bitOffset == 2);

  // The derived ids are part of the list of interrupts and resolve through their canonical interrupt path.
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({0, 0}) != loi.end());
  BOOST_CHECK(loi.find({0, 2}) != loi.end());
  auto canonical = regs.getBackendRegister("!0:1");
  BOOST_CHECK(canonical.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((canonical.interruptId == std::vector<size_t>{0, 1}));
}

/**********************************************************************************************************************/

// A nested handler's ISR children carry the subhandler's full id, so the children of MY_INTC.SUB1/ISR (handler
// [3, 1]) carry the id [3, 1, <bit position>].
BOOST_AUTO_TEST_CASE(TestDerivedInterruptRegistersNestedSubhandler) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");
  (void)metas;

  auto reg = regs.getBackendRegister("/MY_INTC/SUB1/ISR/READY");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{3, 1, 0}));
  BOOST_REQUIRE(reg.channels.size() == 1);
  BOOST_TEST(reg.channels[0].bitOffset == 0);
  BOOST_TEST(reg.channels[0].width == 1);

  auto reg2 = regs.getBackendRegister("/MY_INTC/SUB1/ISR/ERROR");
  BOOST_CHECK(reg2.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg2.interruptId == std::vector<size_t>{3, 1, 1}));
  BOOST_REQUIRE(reg2.channels.size() == 1);
  BOOST_TEST(reg2.channels[0].bitOffset == 1);

  // The nested id resolves through its canonical interrupt path.
  auto canonical = regs.getBackendRegister("!3:1:0");
  BOOST_CHECK(canonical.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((canonical.interruptId == std::vector<size_t>{3, 1, 0}));
}

/**********************************************************************************************************************/

// Helper that writes a modified copy of simpleJsonFile.jmap to <outFile> and parses it, so the negative derivation
// cases below are built by injecting the fault into the full map from code instead of keeping a set of
// near-identical variant fixtures.
static std::pair<ChimeraTK::NumericAddressedRegisterCatalogue, ChimeraTK::MetadataCatalogue> parseInjectedInterruptMap(
    const std::string& outFile, const nlohmann::json& map) {
  std::ofstream(outFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(outFile);
}

/**********************************************************************************************************************/

// A child of ISR that is not a 1-bit bit field has no defined meaning for the handler, so it is rejected at parse time.
BOOST_AUTO_TEST_CASE(TestIsrChildNotOneBit) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Replace one 1-bit child of DAQ/ISR with a 3-bit value.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["representation"] = {
      {"bitShift", 0}, {"width", 3}};

  std::string tmpFile = "isrChildNotOneBit_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A multi-element child of ISR is not one 1-bit bit field, so it is rejected at parse time. The child carries its own
// address node (equal to ISR's) so the parser applies its numberOfElements, which keeps this case independent of the
// own-address rejection below.
BOOST_AUTO_TEST_CASE(TestIsrChildMultipleElements) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give one 1-bit child of DAQ/ISR more than one element, at ISR's own address.
  nlohmann::json& child = map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"];
  child["numberOfElements"] = 2;
  child["address"] = {{"channel", 5}, {"offset", 8}};

  std::string tmpFile = "isrChildMultipleElements_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A child of ISR declared with a channels dictionary is not one 1-bit bit field, so it is rejected at parse time. A
// channels dictionary makes the child itself a multi-channel register and additionally creates a deeper-level slice
// register per named channel, so the no-deeper-levels rule fires; either way a channels child never qualifies.
BOOST_AUTO_TEST_CASE(TestIsrChildWithChannels) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Replace one 1-bit child of DAQ/ISR with a two-channel register at ISR's own address. Both channels are 1 bit, so
  // the channel width does not reject it.
  nlohmann::json& child = map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"];
  child.erase("representation");
  child["address"] = {{"channel", 5}, {"offset", 8}};
  child["channels"] = {
      {"A", {{"bytesPerElement", 4}, {"offset", 0}, {"representation", {{"bitShift", 0}, {"width", 1}}}}},
      {"B", {{"bytesPerElement", 4}, {"offset", 0}, {"representation", {{"bitShift", 0}, {"width", 1}}}}}};

  std::string tmpFile = "isrChildWithChannels_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A child of ISR at its own address rather than ISR's is not one 1-bit bit field, so it is rejected at parse time.
BOOST_AUTO_TEST_CASE(TestIsrChildOwnAddress) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give one 1-bit child of DAQ/ISR its own address, different from ISR's (same channel, different offset).
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["address"] = {{"channel", 5}, {"offset", 16}};

  std::string tmpFile = "isrChildOwnAddress_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A child of ISR on a different channel (bar) from ISR is not one 1-bit bit field, so it is rejected at parse time.
// The child keeps ISR's offset, so only the bar differs and this case is independent of the own-address rejection.
BOOST_AUTO_TEST_CASE(TestIsrChildOwnBar) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give one 1-bit child of DAQ/ISR its own address on a different channel but at ISR's offset.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["address"] = {{"channel", 4}, {"offset", 8}};

  std::string tmpFile = "isrChildOwnBar_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A register below ISR that has children of its own is a deeper level, which the handler does not define.
BOOST_AUTO_TEST_CASE(TestIsrDeeperLevel) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give one child of DAQ/ISR a child of its own.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["children"] = {
      {"DEEPER", {{"representation", {{"bitShift", 0}, {"width", 1}}}}}};

  std::string tmpFile = "isrDeeperLevel_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A multi-element ISR register is rejected at parse time: a channel slice below it would be indistinguishable from a
// bit field.
BOOST_AUTO_TEST_CASE(TestIsrMultipleElements) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  map["addressSpace"]["DAQ"]["children"]["ISR"]["numberOfElements"] = 2;

  std::string tmpFile = "isrMultipleElements_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// An ISR declared with a channels dictionary is rejected at parse time as well: even a single-channel dictionary is
// not a scalar register, because the channel's width is its own width rather than the element pitch.
BOOST_AUTO_TEST_CASE(TestIsrWithChannels) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  nlohmann::json& isr = map["addressSpace"]["DAQ"]["children"]["ISR"];
  isr.erase("children");
  // A single named channel of 16 bits: one channel and one element, but it does not span the 32-bit element word.
  isr["channels"] = {
      {"ONLY", {{"bytesPerElement", 4}, {"offset", 0}, {"representation", {{"width", 16}, {"type", "fixedPoint"}}}}}};

  std::string tmpFile = "isrWithChannels_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A double-buffered ISR is an error at parse time: the auto-generated BUF0/BUF1 buffer views are children of ISR
// that are not bit fields, so the child scan rejects them.
BOOST_AUTO_TEST_CASE(TestIsrDoubleBuffered) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give DAQ/ISR a doubleBuffering block, reusing the DAQ.DOUBLE_BUF enable/read-buffer registers.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["doubleBuffering"] = {
      {"secondaryBufferAddress", {{"type", "IO"}, {"channel", 5}, {"offset", 12}}},
      {"enableRegister", "DAQ.DOUBLE_BUF.ENA"}, {"readBufferRegister", "DAQ.DOUBLE_BUF.INACTIVE_BUF_ID"}};

  std::string tmpFile = "isrDoubleBuffered_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// An ISR without any child is not an error: it parses and the catalogue is unchanged.
BOOST_AUTO_TEST_CASE(TestIsrWithoutChildrenParses) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  map["addressSpace"]["DAQ"]["children"]["ISR"].erase("children");

  std::string tmpFile = "isrWithoutChildren_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto isr = regs.getBackendRegister("/DAQ/ISR");
  BOOST_CHECK(isr.registerAccess != NumericAddressedRegisterInfo::Access::INTERRUPT);
  // No derived ids below DAQ/ISR are present.
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({0, 0}) == loi.end());
  BOOST_CHECK(loi.find({0, 1}) == loi.end());
  BOOST_CHECK(loi.find({0, 2}) == loi.end());
  // The remaining declared ids are untouched.
  BOOST_CHECK(loi.find({0}) != loi.end());
  BOOST_CHECK(loi.find({3, 1, 0}) != loi.end());
}

/**********************************************************************************************************************/

// An INTC handler whose <path>/ISR does not exist is an error at parse time, even when the entry is unused.
BOOST_AUTO_TEST_CASE(TestIsrMissing) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Remove the register the DAQ handler [0] refers to.
  map["addressSpace"]["DAQ"]["children"].erase("ISR");

  std::string tmpFile = "isrMissing_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// An INTC handler entry whose <path>/ISR does not exist is an error at parse time for used and unused entries alike.
// The nested subhandler [3, 1] refers to MY_INTC.SUB1/ISR but no register references [3, 1], so a regression that
// validated only used entries would still pass with this missing.
BOOST_AUTO_TEST_CASE(TestIsrMissingUnusedHandler) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Remove the register the unused subhandler [3, 1] refers to.
  map["addressSpace"]["MY_INTC"]["children"]["SUB1"]["children"].erase("ISR");

  std::string tmpFile = "isrMissingUnused_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// Two children covering the same ISR bit position are an error at parse time.
BOOST_AUTO_TEST_CASE(TestIsrDuplicateBitPosition) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Make the second child cover the same bit as the first.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["SECOND_INTERRUPT"]["representation"] = {
      {"bitShift", 0}, {"width", 1}};

  std::string tmpFile = "isrDuplicateBitPosition_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A derived id that is already present is not an error: two registers may share one interrupt id and are notified
// together by the same ISR bit.
BOOST_AUTO_TEST_CASE(TestSharedInterruptId) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Add a declared interrupt register carrying the derived id [0, 0]. A void interrupt register needs a parent that
  // is a register, so BSP (itself a register) is used rather than the DAQ module.
  map["addressSpace"]["BSP"]["children"]["SHARED_INTERRUPT"] = {
      {"triggeredByInterrupt", {0, 0}}, {"representation", {{"type", "void"}}}};

  std::string tmpFile = "sharedInterruptId_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  // Both the declared register and the derived ISR child carry the shared id.
  auto declared = regs.getBackendRegister("/BSP/SHARED_INTERRUPT");
  BOOST_TEST((declared.interruptId == std::vector<size_t>{0, 0}));
  auto derived = regs.getBackendRegister("/DAQ/ISR/DAQ_READY");
  BOOST_TEST((derived.interruptId == std::vector<size_t>{0, 0}));
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({0, 0}) != loi.end());
}

/**********************************************************************************************************************/

// A child of ISR whose bit position lies outside the ISR word or outside the distributor's 32-bit interrupt mask is an
// error at parse time: the distributor maps an interrupt id to one bit of a 32-bit mask, so a position outside either
// range has no interrupt to notify. The child inherits its address from the ISR parent, so the qualifying-child check
// does not catch it. A 32-bit ISR reaches both range parts at bit position 32; a beyond-word position is not
// separately testable, since a wider ISR word would make the child widen the register's channel past its element
// pitch and trip the scalar-ISR check first.
BOOST_AUTO_TEST_CASE(TestIsrBitPositionOutOfRange) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A 1-bit child at bit position 32 of the 32-bit ISR word: still a qualifying bit field, but the last id element
  // would be used to shift a 32-bit mask by 32.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["representation"] = {
      {"bitShift", 32}, {"width", 1}};

  std::string tmpFile = "isrBitPositionOutOfRange_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// The children of ISR are the registers whose path carries ISR as its complete leading path components. A register
// that only shares the text prefix, such as DAQ/ISR_EXTRA/READY against DAQ/ISR, is not a child: the map parses and
// the register is left untouched. A text-prefix match would turn it into an interrupt or reject it as a deeper level.
BOOST_AUTO_TEST_CASE(TestIsrTextPrefixNotAChild) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A register at the ISR address shaped like a bit field, but under a path that only shares the text prefix of ISR.
  map["addressSpace"]["DAQ"]["children"]["ISR_EXTRA"] = {
      {"children", {{"READY", {{"representation", {{"bitShift", 0}, {"width", 1}}}}}}}, {"bytesPerElement", 4},
      {"numberOfElements", 1}, {"address", {{"channel", 5}, {"offset", 8}}}};

  std::string tmpFile = "isrTextPrefix_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  // The unrelated register keeps its own (non-interrupt) access and is not part of the derived interrupt ids.
  auto reg = regs.getBackendRegister("/DAQ/ISR_EXTRA/READY");
  BOOST_CHECK(reg.registerAccess != NumericAddressedRegisterInfo::Access::INTERRUPT);
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({0, 0}) != loi.end()); // the real ISR child still derives
  for(auto const& id : loi) {
    BOOST_TEST((id != std::vector<size_t>{0, 0, 0}));
  }
}

/**********************************************************************************************************************/

// A qualifying child of ISR that already declares its own interrupt is an error at parse time: an ISR child is
// derived, so a declared interrupt is redundant and the child's own id would be silently overwritten. The check is
// unconditional, so it rejects a declared id equal to the derived one too.
BOOST_AUTO_TEST_CASE(TestIsrChildDeclaredInterruptDifferentId) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give the bit-0 child of DAQ/ISR its own interrupt with a different id.
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["triggeredByInterrupt"] = {7, 5};

  std::string tmpFile = "isrChildDeclaredInterruptDiffId_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A declared interrupt on an ISR child is rejected even when its id equals the derived one: the check tests the
// register access, not the id, so a declared interrupt is redundant rather than merely conflicting.
BOOST_AUTO_TEST_CASE(TestIsrChildDeclaredInterruptSameId) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Give the bit-0 child of DAQ/ISR its own interrupt with the id it would derive anyway ([0, 0]).
  map["addressSpace"]["DAQ"]["children"]["ISR"]["children"]["DAQ_READY"]["triggeredByInterrupt"] = {0, 0};

  std::string tmpFile = "isrChildDeclaredInterruptSameId_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// An interrupt handler entry must carry exactly one controller key at its top level, once 'subhandler' and the
// underscore-prefixed keys are dropped. An entry with several controller keys is a map authoring error and the public
// parser must reject it. The base map is the full fixture, so the INTC key resolves DAQ/ISR and the extra key is an
// unregistered type the dispatcher skips: without the entry-level checks the map would parse. The entry reader and the
// handler augmentation both enforce the invariant, so the case is rejected while either check is present and only
// disappears once both are gone; it exercises the parse contract rather than either check in isolation.
BOOST_AUTO_TEST_CASE(TestInterruptHandlerSeveralControllerKeys) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Add a second, non-underscore controller key beside the existing INTC key of the DAQ handler [0].
  map["interruptHandler"]["0"]["OTHER_TYPE"] = {{"path", "DAQ"}};

  std::string tmpFile = "interruptHandlerSeveralKeys_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// An interrupt handler entry is passed through as metadata regardless of its controller type and data structure: a
// backend-specific "MyINTC" controller carrying an arbitrary structure that is incompatible with the fixed "INTC"
// controller parses and reaches the metadata, normalised only per the common rules (underscore-prefixed keys dropped,
// a missing 'version' defaulted to 1) and re-serialised with json::dump() without the 'subhandler' member.
BOOST_AUTO_TEST_CASE(TestBackendSpecificInterruptHandler) {
  // Build a map with a single interrupt handler entry whose "MyINTC" controller carries a nested object and
  // unexpected value types (a boolean, a floating point, an array with mixed and null entries) that the old fixed
  // "INTC" controller would have dropped. Underscore-prefixed keys (a "_comment" at the top level and inside the
  // controller) must be dropped by the common normalisation.
  nlohmann::json entry = {{"_comment", "dropped top level"},
      {"MyINTC",
          {{"path", "MY_INTC"}, {"cfg", {{"mode", "triggered"}, {"count", 3}, {"flag", true}, {"ratio", 1.5}}},
              {"list", nlohmann::json::array({"a", 2, nullptr})}, {"_comment", "dropped in controller"}}}};
  nlohmann::json subEntry = {{"_comment", "dropped in subhandler"},
      {"MyINTC", {{"path", "MY_INTC.SUB0"}, {"extra", {{"deep", {{"k", "v"}}}}}}}};
  entry["subhandler"]["0"] = subEntry;

  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"]["4"] = entry;
  map["metadata"] = nlohmann::json::object();
  map["addressSpace"] = nlohmann::json::object();

  std::string tmpFile = "backendSpecificInterruptHandler_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  auto [regs, metas] = ChimeraTK::MapFileParser::parse(tmpFile);

  // Expected metadata values: the entry minus its 'subhandler' member, normalised per the common rules (underscore
  // keys dropped, a missing 'version' defaulted to 1), re-serialised with json::dump().
  nlohmann::json expectedTop = entry;
  expectedTop.erase("_comment");
  expectedTop.erase("subhandler");
  expectedTop["MyINTC"].erase("_comment");
  expectedTop["MyINTC"]["version"] = 1;
  BOOST_TEST(metas.getMetadata("![4]") == expectedTop.dump());

  // The subhandler recursion appends its id component and normalises its own entry in the same way.
  nlohmann::json expectedSub = subEntry;
  expectedSub.erase("_comment");
  expectedSub["MyINTC"]["version"] = 1;
  BOOST_TEST(metas.getMetadata("![4,0]") == expectedSub.dump());

  std::remove(tmpFile.c_str());
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

/**********************************************************************************************************************/

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
// simpleJsonFile.jmap DAQ.MUXED_WITH_STATUS, which combines a per-channel 'selectedBy' with a 'children'
// dictionary). Every child slice carries the same register+value selector as the parent channel slice.
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
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  // APP.DATA gates on a selector register 'APP.NONEXISTENT' that is never declared.
  map["addressSpace"]["APP"]["children"]["DATA"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"selectedBy", {{"register", "APP.NONEXISTENT"}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 4}}},
      {"representation", {{"width", 32}}}};
  std::string tmpFile = "selectedBySelectorNotFound_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// S2: a selector register that is itself conditionally enabled by a 'selectedBy' of its own must be rejected: a
// selector drives the gate of another register and must therefore be readable unconditionally.
BOOST_AUTO_TEST_CASE(TestSelectedBySelectorItselfGated) {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  // APP.SEL is itself gated by APP.MASTER_SEL, and APP.DATA gates on APP.SEL. The selector APP.SEL must not be gated.
  map["addressSpace"]["APP"]["children"]["SEL"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"selectedBy", {{"register", "APP.MASTER_SEL"}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 0}}},
      {"representation", {{"width", 32}}}};
  map["addressSpace"]["APP"]["children"]["MASTER_SEL"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"address", {{"channel", 0}, {"offset", 8}}}, {"representation", {{"width", 32}}}};
  map["addressSpace"]["APP"]["children"]["DATA"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"selectedBy", {{"register", "APP.SEL"}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 4}}},
      {"representation", {{"width", 32}}}};
  std::string tmpFile = "selectedBySelectorItselfGated_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/
// Helper used by the selectedBy read-only restriction tests (PR1-PR5): build a minimal map in a temp file and parse
// it. The map carries a selector register APP.SEL and a register APP.DATA whose 'selectedBy' points to it; the
// register's access is controlled by the caller. If 'access' is empty the 'access' member is omitted, defaulting to
// READ_WRITE at parse time.
static ChimeraTK::NumericAddressedRegisterCatalogue parseSelectedByAccessFixture(
    const std::string& access, const nlohmann::json& dataExtra = nlohmann::json::object()) {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  map["addressSpace"]["APP"]["children"]["SEL"] = {
      {"numberOfElements", 1}, {"address", {{"channel", 0}, {"offset", 0}}}, {"representation", {{"width", 32}}}};
  nlohmann::json reg = {{"numberOfElements", 1}, {"selectedBy", {{"register", "APP.SEL"}, {"value", 1}}},
      {"address", {{"channel", 0}, {"offset", 4}}}, {"representation", {{"width", 32}}}};
  if(!access.empty()) {
    reg["access"] = access;
  }
  for(auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
    reg[it.key()] = it.value();
  }
  map["addressSpace"]["APP"]["children"]["DATA"] = reg;
  std::string tmpFile = "selectedByAccess_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(tmpFile).first;
}

/**********************************************************************************************************************/

// PR1: 'selectedBy' on a register with Access::READ_WRITE must be rejected as a parsing error.
BOOST_AUTO_TEST_CASE(TestSelectedByReadWriteRejected) {
  BOOST_CHECK_THROW(parseSelectedByAccessFixture("RW"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// PR2: 'selectedBy' on a register with Access::WRITE_ONLY must be rejected as a parsing error.
BOOST_AUTO_TEST_CASE(TestSelectedByWriteOnlyRejected) {
  BOOST_CHECK_THROW(parseSelectedByAccessFixture("WO"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// PR3: 'selectedBy' on a register with Access::READ_ONLY is accepted.
BOOST_AUTO_TEST_CASE(TestSelectedByReadOnlyAccepted) {
  auto regs = parseSelectedByAccessFixture("RO");
  auto reg = regs.getBackendRegister("/APP/DATA");
  BOOST_REQUIRE(reg.channels[0].selectedBy);
  BOOST_TEST(reg.channels[0].selectedBy->regPath == "/APP/SEL");
  BOOST_TEST(reg.channels[0].selectedBy->val == 1);
}

/**********************************************************************************************************************/

// PR4: 'selectedBy' on a register with Access::INTERRUPT is accepted. An interrupt register is expressed via
// 'triggeredByInterrupt' (which implies read-only) instead of 'access'.
BOOST_AUTO_TEST_CASE(TestSelectedByInterruptAccepted) {
  auto regs = parseSelectedByAccessFixture("", {{"triggeredByInterrupt", nlohmann::json::array({0})}});
  auto reg = regs.getBackendRegister("/APP/DATA");
  BOOST_REQUIRE(reg.channels[0].selectedBy);
  BOOST_TEST(reg.channels[0].selectedBy->val == 1);
}

/**********************************************************************************************************************/

// Helper used by PR5: build a map with a module 'MOD' that declares 'selectedBy', and a single descendant register
// '<childName>' under it. The child's access is controlled by 'childAccess' (empty => READ_WRITE default).
static ChimeraTK::NumericAddressedRegisterCatalogue parseSelectedByInheritedAccessFixture(
    const std::string& childAccess, const std::string& childName) {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  map["addressSpace"]["APP"]["children"]["SEL"] = {
      {"numberOfElements", 1}, {"address", {{"channel", 0}, {"offset", 0}}}, {"representation", {{"width", 32}}}};
  nlohmann::json child = {
      {"numberOfElements", 1}, {"address", {{"channel", 0}, {"offset", 4}}}, {"representation", {{"width", 32}}}};
  if(!childAccess.empty()) {
    child["access"] = childAccess;
  }
  map["addressSpace"]["APP"]["children"]["MOD"] = {
      {"selectedBy", {{"register", "APP.SEL"}, {"value", 1}}}, {"children", {{childName, child}}}};
  std::string tmpFile = "selectedByInheritedAccess_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(tmpFile).first;
}

// PR5: a module/parent declaring 'selectedBy' must be rejected if any descendant register inheriting it is writable;
// the error points to the parent declaration, not the child.
BOOST_AUTO_TEST_CASE(TestSelectedByInheritedWritableDescendantRejected) {
  // A writable (default READ_WRITE) descendant inheriting the module's selectedBy is rejected.
  BOOST_CHECK_THROW(parseSelectedByInheritedAccessFixture("", "CHILD"), ChimeraTK::logic_error);
  // An explicitly writable descendant is rejected as well.
  BOOST_CHECK_THROW(parseSelectedByInheritedAccessFixture("WO", "CHILD"), ChimeraTK::logic_error);
  // A read-only descendant satisfies the constraint.
  auto regs = parseSelectedByInheritedAccessFixture("RO", "CHILD");
  auto reg = regs.getBackendRegister("/APP/MOD/CHILD");
  BOOST_REQUIRE(reg.channels[0].selectedBy);
  BOOST_TEST(reg.channels[0].selectedBy->val == 1);
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

// All mapFormatVersion checks: a file declaring the supported version "1.0" parses without error, and any other
// version throws ChimeraTK::logic_error.
BOOST_AUTO_TEST_CASE(TestMapFormatVersionWithPatch) {
  BOOST_CHECK_NO_THROW(parseWithMapFormatVersion("mapFormatVersionSupported.jmap", "1.0"));
  BOOST_CHECK_NO_THROW(parseWithMapFormatVersion("mapFormatVersionLeadingZerosMinor.jmap", "1.00"));
  BOOST_CHECK_NO_THROW(parseWithMapFormatVersion("mapFormatVersionLeadingZerosMajor.jmap", "01.0"));
  BOOST_CHECK_NO_THROW(parseWithMapFormatVersion("mapFormatVersionLeadingZeros.jmap", "01.00"));

  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionOld.jmap", "0.0.1"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionMismatchedMinor.jmap", "1.1"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionMismatchedMajor.jmap", "2.0"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionSingle.jmap", "1"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionMissingMajor.jmap", ".0"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionMissingMinor.jmap", "1."), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionMajorNoDot.jmap", "1x.0"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionNonDecimal.jmap", "1.0 "), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionOversizedMinor.jmap", "1.999999999999999999999999"),
      ChimeraTK::logic_error);
  BOOST_CHECK_THROW(parseWithMapFormatVersion("mapFormatVersionOversizedMajor.jmap", "999999999999999999999999.0"),
      ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      parseWithMapFormatVersion("mapFormatVersionMissing.jmap", nlohmann::json(nullptr)), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      parseWithMapFormatVersion("mapFormatVersionWrongType.jmap", nlohmann::json(1.0)), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// 'triggeredByInterrupt' may name another interrupt register instead of giving the numeric id array. A void register
// is read-only with wait_for_new_data exactly as for a numeric id, and it may carry a register-name reference: it
// takes the target's resolved id, appears in the list of interrupts and resolves through the canonical interrupt path.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceVoidReferrer) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A void register (no address) referring by name to the void interrupt register BSP/VOID_INTERRUPT_0 (id [0]).
  map["addressSpace"]["BSP"]["children"]["VOID_REFERRING"] = {
      {"triggeredByInterrupt", "BSP.VOID_INTERRUPT_0"}, {"representation", {{"type", "void"}}}};

  std::string tmpFile = "interruptRefVoidReferrer_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto reg = regs.getBackendRegister("/BSP/VOID_REFERRING");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{0}));

  // The void referring register is part of the list of interrupts and resolves through its canonical interrupt path.
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({0}) != loi.end());
  auto canonical = regs.getBackendRegister("!0");
  BOOST_CHECK(canonical.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((canonical.interruptId == std::vector<size_t>{0}));
}

/**********************************************************************************************************************/

// 'triggeredByInterrupt' may name another interrupt register instead of giving the numeric id array. A data register
// referring to a void interrupt register takes that register's resolved id: it appears in the list of interrupts and
// resolves through the canonical interrupt path, exactly as if it had declared the id itself.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceToVoidRegister) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // BSP/SOME_INFO is a data register; refer by name to the void interrupt register BSP/VOID_INTERRUPT_0 (id [0]).
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "BSP.VOID_INTERRUPT_0"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}},
      {"_", "^^^ inside BSP, so the address offset is relative to BSP's address"}};

  std::string tmpFile = "interruptRefVoid_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto reg = regs.getBackendRegister("/BSP/REFERRING");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{0}));

  // The referring register is part of the list of interrupts and resolves through its canonical interrupt path.
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({0}) != loi.end());
  auto canonical = regs.getBackendRegister("!0");
  BOOST_CHECK(canonical.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((canonical.interruptId == std::vector<size_t>{0}));
}

/**********************************************************************************************************************/

// The target of a reference may carry data: triggering does not depend on the target's data, so a non-void target is
// allowed and the referring register takes the target's resolved id.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceToDataRegister) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A void interrupt register with id [7], which is itself data-carrying (a fixed-point representation).
  map["addressSpace"]["BSP"]["children"]["DATA_INTERRUPT"] = {{"triggeredByInterrupt", {7}}, {"numberOfElements", 1},
      {"bytesPerElement", 4}, {"address", {{"offset", 20}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}},
      {"_", "^^^ a data-carrying interrupt register: the trigger only uses its id"}};
  // A data register referring to the data-carrying interrupt register by name.
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "BSP.DATA_INTERRUPT"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 24}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefData_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto target = regs.getBackendRegister("/BSP/DATA_INTERRUPT");
  BOOST_CHECK(target.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((target.interruptId == std::vector<size_t>{7}));

  auto reg = regs.getBackendRegister("/BSP/REFERRING");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{7}));
  auto loi = regs.getListOfInterrupts();
  BOOST_CHECK(loi.find({7}) != loi.end());
}

/**********************************************************************************************************************/

// A reference resolves after the implicit interrupt ids have been augmented, so a target may be a derived ISR child.
// The reference names the derived child by its named path and takes the id the augmentation gave it.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceToDerivedIsrChild) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // The INTC handler [0] refers to DAQ/ISR; its bit-0 child DAQ_READY derives the id [0, 0].
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "DAQ.ISR.DAQ_READY"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefDerived_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto derived = regs.getBackendRegister("/DAQ/ISR/DAQ_READY");
  BOOST_CHECK(derived.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((derived.interruptId == std::vector<size_t>{0, 0}));

  auto reg = regs.getBackendRegister("/BSP/REFERRING");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{0, 0}));
}

/**********************************************************************************************************************/

// A reference is resolved in a post-pass over the whole catalogue, so the target may be declared after the referring
// register in the file.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceForward) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // The referring register is declared first; the target BSP/TARGET_VOID_INT afterwards.
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "BSP.TARGET_VOID_INT"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};
  map["addressSpace"]["BSP"]["children"]["TARGET_VOID_INT"] = {
      {"triggeredByInterrupt", {9}}, {"representation", {{"type", "void"}}}};

  std::string tmpFile = "interruptRefForward_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto reg = regs.getBackendRegister("/BSP/REFERRING");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((reg.interruptId == std::vector<size_t>{9}));
}

/**********************************************************************************************************************/

// A reference resolves transitively: a referring register names a target that itself names another register, and the
// final id is the one at the end of the chain.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceChain) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // Chain: REFERRING -> INTERMEDIATE -> FINAL (void, id [4]).
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "BSP.INTERMEDIATE"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};
  map["addressSpace"]["BSP"]["children"]["INTERMEDIATE"] = {{"triggeredByInterrupt", "BSP.FINAL"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 20}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};
  map["addressSpace"]["BSP"]["children"]["FINAL"] = {
      {"triggeredByInterrupt", {4}}, {"representation", {{"type", "void"}}}};

  std::string tmpFile = "interruptRefChain_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  BOOST_TEST((regs.getBackendRegister("/BSP/FINAL").interruptId == std::vector<size_t>{4}));
  BOOST_TEST((regs.getBackendRegister("/BSP/INTERMEDIATE").interruptId == std::vector<size_t>{4}));
  BOOST_TEST((regs.getBackendRegister("/BSP/REFERRING").interruptId == std::vector<size_t>{4}));
}

/**********************************************************************************************************************/

// A reference cycle is an error at parse time. Two registers naming each other form a cycle.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceCycle) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  map["addressSpace"]["BSP"]["children"]["FIRST"] = {{"triggeredByInterrupt", "BSP.SECOND"}, {"numberOfElements", 1},
      {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};
  map["addressSpace"]["BSP"]["children"]["SECOND"] = {{"triggeredByInterrupt", "BSP.FIRST"}, {"numberOfElements", 1},
      {"bytesPerElement", 4}, {"address", {{"offset", 20}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefCycle_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A register naming itself is a cycle and an error at parse time.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceSelf) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  map["addressSpace"]["BSP"]["children"]["SELF"] = {{"triggeredByInterrupt", "BSP.SELF"}, {"numberOfElements", 1},
      {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefSelf_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A reference to a register that does not exist is an error at parse time.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceMissingTarget) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "BSP.DOES_NOT_EXIST"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefMissing_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A reference to a register that is not an interrupt register is an error at parse time.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceNonInterruptTarget) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // BSP/VERSION is an ordinary read-only data register, not an interrupt register.
  map["addressSpace"]["BSP"]["children"]["REFERRING"] = {{"triggeredByInterrupt", "BSP.VERSION"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefNonInterrupt_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

// A 2D register declared with the string form creates one slice register per named channel. Every slice carries the
// resolved interrupt id of the referring register.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceChannelSlice) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A 2D register whose channels A and B refer by name to the void interrupt register BSP/VOID_INTERRUPT_0 (id [0]).
  map["addressSpace"]["BSP"]["children"]["REFERRING_2D"] = {{"triggeredByInterrupt", "BSP.VOID_INTERRUPT_0"},
      {"numberOfElements", 4}, {"pitch", 8}, {"address", {{"type", "DMA"}, {"offset", "0x2000"}}},
      {"channels",
          {{"A", {{"bytesPerElement", 4}, {"offset", 0}, {"representation", {{"type", "fixedPoint"}, {"width", 32}}}}},
              {"B",
                  {{"bytesPerElement", 4}, {"offset", 4},
                      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}}}}}};

  std::string tmpFile = "interruptRefSlice_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  // Both the 2D register and its channel slices carry the resolved id [0].
  BOOST_TEST((regs.getBackendRegister("/BSP/REFERRING_2D").interruptId == std::vector<size_t>{0}));
  auto sliceA = regs.getBackendRegister("/BSP/REFERRING_2D/A");
  BOOST_CHECK(sliceA.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((sliceA.interruptId == std::vector<size_t>{0}));
  auto sliceB = regs.getBackendRegister("/BSP/REFERRING_2D/B");
  BOOST_CHECK(sliceB.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT);
  BOOST_TEST((sliceB.interruptId == std::vector<size_t>{0}));
}

/**********************************************************************************************************************/

// An empty 'triggeredByInterrupt' array counts as absent: no id-less interrupt register is created, so a data register
// keeps its declared access instead of silently becoming an interrupt register with an empty id.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceEmptyArrayIsAbsentForDataRegister) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A data register with an empty id array and a declared read-only access: it must stay read-only.
  map["addressSpace"]["BSP"]["children"]["EMPTY_ARRAY"] = {{"triggeredByInterrupt", nlohmann::json::array()},
      {"access", "RO"}, {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefEmptyArrayData_" + std::to_string(getpid()) + ".jmap";
  auto [regs, metas] = parseInjectedInterruptMap(tmpFile, map);
  (void)metas;
  std::remove(tmpFile.c_str());

  auto reg = regs.getBackendRegister("/BSP/EMPTY_ARRAY");
  BOOST_CHECK(reg.registerAccess == NumericAddressedRegisterInfo::Access::READ_ONLY);
  BOOST_TEST(reg.interruptId.empty());
}

/**********************************************************************************************************************/

// An empty 'triggeredByInterrupt' array counts as absent for a void register as well: it has no address and no id, so
// the entry is rejected with the same error as a void register without a 'triggeredByInterrupt' entry.
BOOST_AUTO_TEST_CASE(TestInterruptReferenceEmptyArrayIsAbsentForVoidRegister) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A void register with an empty id array: it declares no interrupt, so parsing fails cleanly.
  map["addressSpace"]["BSP"]["children"]["EMPTY_ARRAY_VOID"] = {
      {"triggeredByInterrupt", nlohmann::json::array()}, {"representation", {{"type", "void"}}}};

  std::string tmpFile = "interruptRefEmptyArrayVoid_" + std::to_string(getpid()) + ".jmap";
  BOOST_CHECK_THROW(parseInjectedInterruptMap(tmpFile, map), ChimeraTK::logic_error);
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
