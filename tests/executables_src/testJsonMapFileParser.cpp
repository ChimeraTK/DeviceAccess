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

// Helper used by the gatedBy fault tests below: load simpleJsonFile.jmap as the base map, overwrite the
// gatedBy of the existing COLLISION/FD/Sel0 channel with the supplied (possibly malformed) one, write the result
// to <outFile>, then parse it. This replaces the standalone gatedBy*.jmap fixtures: the fault is injected into an
// existing entry of the full simpleJsonFile map (via nlohmann-json) and the produced file is what gets parsed, so no
// separate fault-only map files need to be kept.
static std::pair<ChimeraTK::NumericAddressedRegisterCatalogue, ChimeraTK::MetadataCatalogue> parseInjectedGatedByFault(
    const std::string& outFile, const nlohmann::json& gatedBy) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);

  // Overwrite the gatedBy of an existing channel (COLLISION/FD/Sel0) already carrying one, so the fault is
  // injected into an existing entry rather than introducing a new register.
  map["addressSpace"]["COLLISION"]["children"]["FD"]["channels"]["Sel0"]["gatedBy"] = gatedBy;

  std::ofstream(outFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(outFile);
}

// Helper used by the map format version tests below: load simpleJsonFile.jmap as the base map, set the top-level
// mapFormatVersion entry to the supplied value (or remove it when the value is null), write the result to <outFile>,
// then parse it. As with the gatedBy fault helper above, this avoids keeping a set of near-identical variant
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
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/MUX");
    BOOST_TEST(reg.channels[0].gatedBy->val == 0);
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
      BOOST_TEST(!channel.gatedBy);
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
    // A slice of a non-muxed channel carries no gating
    BOOST_TEST(!reg.channels[0].gatedBy);
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
    // A slice of a non-muxed SIMPLE2D channel carries no gating
    BOOST_TEST(!reg.channels[0].gatedBy);
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
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].gatedBy->val == 0);

    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 32);
    BOOST_TEST(reg.channels[1].nFractionalBits == 0);
    BOOST_TEST(reg.channels[1].signedFlag == true);
    BOOST_REQUIRE(reg.channels[1].gatedBy);
    BOOST_TEST(reg.channels[1].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].gatedBy->val == 1);

    BOOST_TEST(reg.channels[2].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 16);
    BOOST_TEST(reg.channels[2].nFractionalBits == -2);
    BOOST_TEST(reg.channels[2].signedFlag == true);
    BOOST_REQUIRE(reg.channels[2].gatedBy);
    BOOST_TEST(reg.channels[2].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[2].gatedBy->val == 0);

    BOOST_TEST(reg.channels[3].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[3].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[3].width == 32);
    BOOST_TEST(reg.channels[3].nFractionalBits == 0);
    BOOST_TEST(reg.channels[3].signedFlag == true);
    BOOST_REQUIRE(reg.channels[3].gatedBy);
    BOOST_TEST(reg.channels[3].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[3].gatedBy->val == 1);
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
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].gatedBy->val == 0);

    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 32);
    BOOST_TEST(reg.channels[1].nFractionalBits == 0);
    BOOST_TEST(reg.channels[1].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[1].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].gatedBy->val == 1);

    BOOST_TEST(reg.channels[2].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 16);
    BOOST_TEST(reg.channels[2].nFractionalBits == -2);
    BOOST_TEST(reg.channels[2].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[2].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[2].gatedBy->val == 0);

    BOOST_TEST(reg.channels[3].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[3].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[3].width == 32);
    BOOST_TEST(reg.channels[3].nFractionalBits == 0);
    BOOST_TEST(reg.channels[3].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[3].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[3].gatedBy->val == 1);
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
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].gatedBy->val == 0);

    BOOST_TEST(reg.channels[1].bitOffset == 0);
    BOOST_CHECK(reg.channels[1].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[1].width == 32);
    BOOST_TEST(reg.channels[1].nFractionalBits == 0);
    BOOST_TEST(reg.channels[1].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[1].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[1].gatedBy->val == 1);

    BOOST_TEST(reg.channels[2].bitOffset == 2 * 8);
    BOOST_CHECK(reg.channels[2].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[2].width == 16);
    BOOST_TEST(reg.channels[2].nFractionalBits == -2);
    BOOST_TEST(reg.channels[2].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[2].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[2].gatedBy->val == 0);

    BOOST_TEST(reg.channels[3].bitOffset == 4 * 8);
    BOOST_CHECK(reg.channels[3].dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT);
    BOOST_TEST(reg.channels[3].width == 32);
    BOOST_TEST(reg.channels[3].nFractionalBits == 0);
    BOOST_TEST(reg.channels[3].signedFlag == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[3].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[3].gatedBy->val == 1);
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

  auto loi = regs.getListOfInterrupts();
  BOOST_TEST(loi.size() == 3);
  BOOST_CHECK(loi.find({0}) != loi.end());
  BOOST_CHECK(loi.find({3, 0, 1}) != loi.end());
  BOOST_CHECK(loi.find({1}) != loi.end());
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

// A bit-field child slice of a muxed channel inherits the channel's gatedBy condition (fixture
// simpleJsonFile.jmap DAQ.MUXED_WITH_STATUS, which combines a per-channel 'gatedBy' with a 'children'
// dictionary). Every child slice carries the same register+value gating as the parent channel slice.
BOOST_AUTO_TEST_CASE(TestNamedChannelBitRangeMuxedChildGatedBy) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");

  // The muxed parent channel slice inherits the gatedBy condition.
  {
    auto reg = regs.getBackendRegister("DAQ.MUXED_WITH_STATUS.StatusCh0");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].gatedBy->val == 2);
    BOOST_TEST(reg.isBitRange == false);
  }
  // Each bit-field child slice of the muxed channel inherits the channel's gatedBy condition.
  {
    auto reg = regs.getBackendRegister("DAQ.MUXED_WITH_STATUS.StatusCh0.ProbeLimiter");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 0);
    BOOST_TEST(reg.channels[0].width == 1);
    BOOST_TEST(reg.isBitRange == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].gatedBy->val == 2);
  }
  {
    auto reg = regs.getBackendRegister("DAQ.MUXED_WITH_STATUS.StatusCh0.ErrorCounter");
    BOOST_REQUIRE(reg.channels.size() == 1);
    BOOST_TEST(reg.channels[0].bitOffset == 2);
    BOOST_TEST(reg.channels[0].width == 3);
    BOOST_TEST(reg.isBitRange == true);
    BOOST_REQUIRE(reg.channels[0].gatedBy);
    BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
    BOOST_TEST(reg.channels[0].gatedBy->val == 2);
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

// ChannelInfo::operator== (and !=) must include the gatedBy member. Two channel infos
// that differ only in their gating must compare unequal.
BOOST_AUTO_TEST_CASE(TestChannelInfoEqualityGatedBy) {
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
    c.gatedBy = std::nullopt;
  };
  initChannelInfo(a);
  initChannelInfo(b);

  // Two identically initialised infos are equal (both ungated: empty register, default value).
  BOOST_CHECK(a == b);
  BOOST_CHECK(!(a != b));

  BOOST_REQUIRE(!a.gatedBy);
  BOOST_REQUIRE(!b.gatedBy);

  // Differing only in gatedByRegister -> unequal.
  b.gatedBy.emplace(RegisterPath("/DAQ/MUX_SEL"), 0);
  BOOST_CHECK(a != b);
  BOOST_CHECK(!(a == b));

  // Same register again -> equal.
  a.gatedBy.emplace(RegisterPath("/DAQ/MUX_SEL"), 0);
  BOOST_CHECK(a == b);

  // Differing only in gatedByValue -> unequal.
  b.gatedBy->val = 1;
  BOOST_CHECK(a != b);
  BOOST_CHECK(!(a == b));
}

/**********************************************************************************************************************/

// gatedBy with only 'register', no 'value': both fields are required, so the parser rejects the map with a
// std::logic_error. A missing 'value' must not silently default.
BOOST_AUTO_TEST_CASE(TestGatedByMissingValue) {
  nlohmann::json sel{{"register", "COLLISION.MUX"}}; // missing 'value'
  BOOST_CHECK_THROW(parseInjectedGatedByFault("gatedByMissingValue.jmap", sel), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// gatedBy with only 'value', no 'register': both fields are required, so the parser rejects the map with a
// std::logic_error.
BOOST_AUTO_TEST_CASE(TestGatedByMissingRegister) {
  nlohmann::json sel{{"value", 0}}; // missing 'register'
  BOOST_CHECK_THROW(parseInjectedGatedByFault("gatedByMissingRegister.jmap", sel), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// The gate value is unsigned: a negative 'value' must be rejected by the parser.
BOOST_AUTO_TEST_CASE(TestGatedByNegativeValue) {
  nlohmann::json sel{{"register", "COLLISION.MUX"}, {"value", -1}};
  BOOST_CHECK_THROW(parseInjectedGatedByFault("gatedByNegativeValue.jmap", sel), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

// 'gatedBy' referencing a gating register that does not exist in the catalogue must be rejected.
BOOST_AUTO_TEST_CASE(TestGatedByGatingNotFound) {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  // APP.DATA gates on a gating register 'APP.NONEXISTENT' that is never declared.
  map["addressSpace"]["APP"]["children"]["DATA"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"gatedBy", {{"register", "APP.NONEXISTENT"}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 4}}},
      {"representation", {{"width", 32}}}};
  std::string tmpFile = "gatedByGatingNotFound_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
}

// A gating register that is itself gated by a 'gatedBy' of its own must be rejected: a
// gating drives the gate of another register and must therefore be readable unconditionally.
BOOST_AUTO_TEST_CASE(TestGatedByGatingItselfGated) {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  // APP.SEL is itself gated by APP.MASTER_SEL, and APP.DATA gates on APP.SEL. The gating APP.SEL must not be gated.
  map["addressSpace"]["APP"]["children"]["SEL"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"gatedBy", {{"register", "APP.MASTER_SEL"}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 0}}},
      {"representation", {{"width", 32}}}};
  map["addressSpace"]["APP"]["children"]["MASTER_SEL"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"address", {{"channel", 0}, {"offset", 8}}}, {"representation", {{"width", 32}}}};
  map["addressSpace"]["APP"]["children"]["DATA"] = {{"numberOfElements", 1}, {"access", "RO"},
      {"gatedBy", {{"register", "APP.SEL"}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 4}}},
      {"representation", {{"width", 32}}}};
  std::string tmpFile = "gatedByGatingItselfGated_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  BOOST_CHECK_THROW(ChimeraTK::MapFileParser::parse(tmpFile), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/
// Helper used by the gatedBy read-only restriction tests: build a minimal map in a temp file and parse
// it. The map carries a gating register APP.SEL and a register APP.DATA whose 'gatedBy' points to it; the
// register's access is controlled by the caller. If 'access' is empty the 'access' member is omitted, defaulting to
// READ_WRITE at parse time.
static ChimeraTK::NumericAddressedRegisterCatalogue parseGatedByAccessFixture(
    const std::string& access, const nlohmann::json& dataExtra = nlohmann::json::object()) {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  map["addressSpace"]["APP"]["children"]["SEL"] = {
      {"numberOfElements", 1}, {"address", {{"channel", 0}, {"offset", 0}}}, {"representation", {{"width", 32}}}};
  nlohmann::json reg = {{"numberOfElements", 1}, {"gatedBy", {{"register", "APP.SEL"}, {"value", 1}}},
      {"address", {{"channel", 0}, {"offset", 4}}}, {"representation", {{"width", 32}}}};
  if(!access.empty()) {
    reg["access"] = access;
  }
  for(auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
    reg[it.key()] = it.value();
  }
  map["addressSpace"]["APP"]["children"]["DATA"] = reg;
  std::string tmpFile = "gatedByAccess_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(tmpFile).first;
}

/**********************************************************************************************************************/

// 'gatedBy' on a register with Access::READ_WRITE must be rejected as a parsing error.
BOOST_AUTO_TEST_CASE(TestGatedByReadWriteRejected) {
  BOOST_CHECK_THROW(parseGatedByAccessFixture("RW"), ChimeraTK::logic_error);
}

// 'gatedBy' on a register with Access::WRITE_ONLY must be rejected as a parsing error.
BOOST_AUTO_TEST_CASE(TestGatedByWriteOnlyRejected) {
  BOOST_CHECK_THROW(parseGatedByAccessFixture("WO"), ChimeraTK::logic_error);
}

// 'gatedBy' on a register with Access::READ_ONLY is accepted.
BOOST_AUTO_TEST_CASE(TestGatedByReadOnlyAccepted) {
  auto regs = parseGatedByAccessFixture("RO");
  auto reg = regs.getBackendRegister("/APP/DATA");
  BOOST_REQUIRE(reg.channels[0].gatedBy);
  BOOST_TEST(reg.channels[0].gatedBy->regPath == "/APP/SEL");
  BOOST_TEST(reg.channels[0].gatedBy->val == 1);
}

// 'gatedBy' on a register with Access::INTERRUPT is accepted. An interrupt register is expressed via
// 'triggeredByInterrupt' (which implies read-only) instead of 'access'.
BOOST_AUTO_TEST_CASE(TestGatedByInterruptAccepted) {
  auto regs = parseGatedByAccessFixture("", {{"triggeredByInterrupt", nlohmann::json::array({0})}});
  auto reg = regs.getBackendRegister("/APP/DATA");
  BOOST_REQUIRE(reg.channels[0].gatedBy);
  BOOST_TEST(reg.channels[0].gatedBy->val == 1);
}

/**********************************************************************************************************************/

// Helper used by the inherited read-only restriction test: build a map with a module 'MOD' that declares
// 'gatedBy', and a single descendant register
// '<childName>' under it. The child's access is controlled by 'childAccess' (empty => READ_WRITE default).
static ChimeraTK::NumericAddressedRegisterCatalogue parseGatedByInheritedAccessFixture(
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
      {"gatedBy", {{"register", "APP.SEL"}, {"value", 1}}}, {"children", {{childName, child}}}};
  std::string tmpFile = "gatedByInheritedAccess_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  return ChimeraTK::MapFileParser::parse(tmpFile).first;
}

// A module/parent declaring 'gatedBy' must be rejected if any descendant register inheriting it is writable;
// the error points to the parent declaration, not the child.
BOOST_AUTO_TEST_CASE(TestGatedByInheritedWritableDescendantRejected) {
  // A writable (default READ_WRITE) descendant inheriting the module's gatedBy is rejected.
  BOOST_CHECK_THROW(parseGatedByInheritedAccessFixture("", "CHILD"), ChimeraTK::logic_error);
  // An explicitly writable descendant is rejected as well.
  BOOST_CHECK_THROW(parseGatedByInheritedAccessFixture("WO", "CHILD"), ChimeraTK::logic_error);
  // A read-only descendant satisfies the constraint.
  auto regs = parseGatedByInheritedAccessFixture("RO", "CHILD");
  auto reg = regs.getBackendRegister("/APP/MOD/CHILD");
  BOOST_REQUIRE(reg.channels[0].gatedBy);
  BOOST_TEST(reg.channels[0].gatedBy->val == 1);
}

/**********************************************************************************************************************/

// The extended gating-register checks must also apply to a register-level 'gatedBy' on a 2D register, which is stored
// separately in NumericAddressedRegisterInfo::registerGatedBy. Build a 2D register (with a single channel) that
// carries a register-level gate on 'gatingRegister' and parse it. The gating register is declared by the caller.
static void parse2DRegisterGatedByFixture(const std::string& gatingRegisterPath, bool declareGatingRegister,
    bool gatingRegisterItselfGated, const std::string& registerAccess = "RO") {
  nlohmann::json map;
  map["mapFormatVersion"] = "1.0";
  map["interruptHandler"] = nlohmann::json::object();
  map["metadata"] = nlohmann::json::object();
  if(declareGatingRegister) {
    nlohmann::json gating = {
        {"numberOfElements", 1}, {"address", {{"channel", 0}, {"offset", 0}}}, {"representation", {{"width", 32}}}};
    if(gatingRegisterItselfGated) {
      gating["gatedBy"] = {{"register", "APP.MASTER_SEL"}, {"value", 1}};
      map["addressSpace"]["APP"]["children"]["MASTER_SEL"] = {
          {"numberOfElements", 1}, {"address", {{"channel", 0}, {"offset", 8}}}, {"representation", {{"width", 32}}}};
    }
    // A 'gatedBy' may only sit on a read-only register, and the gating register itself must be readable.
    gating["access"] = "RO";
    map["addressSpace"]["APP"]["children"]["GATING"] = gating;
  }
  map["addressSpace"]["APP"]["children"]["REG2D"] = {{"access", registerAccess},
      {"gatedBy", {{"register", gatingRegisterPath}, {"value", 1}}}, {"address", {{"channel", 0}, {"offset", 16}}},
      {"numberOfElements", 4}, {"pitch", 4},
      {"channels",
          {{"C0",
              {{"bytesPerElement", 2}, {"offset", 0}, {"representation", {{"type", "fixedPoint"}, {"width", 16}}}}}}}};
  std::string tmpFile = "gatedBy2DRegisterLevel_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);
  ChimeraTK::MapFileParser::parse(tmpFile);
}

// A register-level 'gatedBy' on a 2D register referencing an undefined gating register is rejected.
BOOST_AUTO_TEST_CASE(TestGatedByRegisterLevelGatingNotFound) {
  BOOST_CHECK_THROW(parse2DRegisterGatedByFixture("APP.NONEXISTENT", /*declareGatingRegister=*/false,
                        /*gatingRegisterItselfGated=*/false),
      ChimeraTK::logic_error);
}

// A register-level 'gatedBy' on a 2D register whose gating register is itself gated is rejected.
BOOST_AUTO_TEST_CASE(TestGatedByRegisterLevelGatingItselfGated) {
  BOOST_CHECK_THROW(parse2DRegisterGatedByFixture("APP.GATING", /*declareGatingRegister=*/true,
                        /*gatingRegisterItselfGated=*/true),
      ChimeraTK::logic_error);
}

// A register-level 'gatedBy' on a 2D register that is writable is rejected.
BOOST_AUTO_TEST_CASE(TestGatedByRegisterLevelWritableRejected) {
  BOOST_CHECK_THROW(parse2DRegisterGatedByFixture("APP.GATING", /*declareGatingRegister=*/true,
                        /*gatingRegisterItselfGated=*/false, /*registerAccess=*/"RW"),
      ChimeraTK::logic_error);
}

// A valid register-level 'gatedBy' on a 2D register is accepted and recorded in registerGatedBy, independent of the
// per-channel gates.
BOOST_AUTO_TEST_CASE(TestGatedByRegisterLevelAccepted) {
  parse2DRegisterGatedByFixture("APP.GATING", /*declareGatingRegister=*/true, /*gatingRegisterItselfGated=*/false);
  auto regs = ChimeraTK::MapFileParser::parse("gatedBy2DRegisterLevel_" + std::to_string(getpid()) + ".jmap").first;
  auto reg = regs.getBackendRegister("/APP/REG2D");
  BOOST_REQUIRE(reg.registerGatedBy);
  BOOST_TEST(reg.registerGatedBy->regPath == "/APP/GATING");
  BOOST_TEST(reg.registerGatedBy->val == 1);
  // The register-level gate is not inherited by the channel slice.
  auto slice = regs.getBackendRegister("/APP/REG2D/C0");
  BOOST_REQUIRE(slice.channels.size() == 1);
  BOOST_CHECK(!slice.channels[0].gatedBy);
}

/**********************************************************************************************************************/

// gatedBy on a non-2D (scalar) register is supported: it makes the single register valid, so the
// register's single channel must carry the gate.
BOOST_AUTO_TEST_CASE(TestGatedByOnScalar) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("simpleJsonFile.jmap");
  auto reg = regs.getBackendRegister("/DAQ/SINGLE_MUXED");
  BOOST_REQUIRE(reg.channels.size() == 1);
  BOOST_REQUIRE(reg.channels[0].gatedBy);
  BOOST_TEST(reg.channels[0].gatedBy->regPath == "/DAQ/MUX_SEL");
  BOOST_TEST(reg.channels[0].gatedBy->val == 0);
  // A register without a gatedBy reports no gate (regression).
  const auto& ungated = regs.getBackendRegister("/DAQ/MACRO_PULSE_NUMBER");
  BOOST_REQUIRE(ungated.channels.size() == 1);
  BOOST_CHECK(!ungated.channels[0].gatedBy);
}

/**********************************************************************************************************************/

// A 'gatedBy' declared on a module or parent register is inherited by all descendant registers/channels that lack
// their own declaration; the nearest ancestor's declaration wins for a node that has its own.
BOOST_AUTO_TEST_CASE(TestGatedByInheritance) {
  auto [regs, metas] = ChimeraTK::MapFileParser::parse("gatedByInheritance.jmap");

  // Helpers: assert the single channel of a scalar/1D register carries the given gate (or none).
  auto checkRegGatedBy = [](const NumericAddressedRegisterInfo& reg, const std::string& expectedReg,
                             std::optional<uint64_t> expectedVal) {
    BOOST_REQUIRE(reg.channels.size() == 1);
    const auto& sb = reg.channels[0].gatedBy;
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
  auto check2DChannelsGatedBy = [](const NumericAddressedRegisterInfo& reg,
                                    std::initializer_list<std::pair<std::string, uint64_t>> expected) {
    BOOST_REQUIRE(reg.channels.size() == expected.size());
    size_t i = 0;
    for(const auto& [expectedReg, expectedVal] : expected) {
      const auto& sb = reg.channels[i].gatedBy;
      BOOST_REQUIRE(sb.has_value());
      BOOST_TEST(sb->regPath == expectedReg);
      BOOST_TEST(sb->val == expectedVal);
      ++i;
    }
  };

  // Module inheritance. REG_A inherits the module default; REG_B keeps its own (override).
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/REG_A"), "/APP/OUTPUT_SELECT", 3);
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/REG_B"), "/CTRL/MODE", 9);

  // Nested inheritance (module -> submodule -> leaf). SUB carries no gatedBy, so LEAF inherits from INHERIT.
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/SUB/LEAF"), "/APP/OUTPUT_SELECT", 3);

  // 2D module default. All channels of a contained 2D register without own declarations inherit the module default.
  check2DChannelsGatedBy(
      regs.getBackendRegister("/INHERIT/CHANA"), {{"/APP/OUTPUT_SELECT", 3}, {"/APP/OUTPUT_SELECT", 3}});

  // Channel override. D1 has its own per-channel gatedBy; D0 inherits the module default.
  check2DChannelsGatedBy(regs.getBackendRegister("/INHERIT/CHANB"), {{"/APP/OUTPUT_SELECT", 3}, {"/CTRL/CHAN_SEL", 7}});

  // Register-module dual role. Its gatedBy applies to its own register AND to its descendant.
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/REG2D_OWN"), "/CTRL/DUAL", 5);
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/REG2D_OWN/CHILD"), "/CTRL/DUAL", 5);

  // Register-level gatedBy directly on a 2D register (no module) is valid: it is recorded in the register's
  // registerGatedBy and is NOT inherited by the channels, which stay ungated.
  {
    const auto& reg2dTop = regs.getBackendRegister("/REG2D_TOP");
    BOOST_REQUIRE(reg2dTop.registerGatedBy.has_value());
    BOOST_TEST(reg2dTop.registerGatedBy->regPath == "/APP/TOP_SEL");
    BOOST_TEST(reg2dTop.registerGatedBy->val == 11);
    for(const auto& channel : reg2dTop.channels) {
      BOOST_CHECK(!channel.gatedBy);
    }
  }

  // A leaf with no ancestor carrying gatedBy stays ungated (no inheritance).
  checkRegGatedBy(regs.getBackendRegister("/LEAF_TOP"), "", std::nullopt);

  // The 1D channel slices of an inherited 2D register also carry the effective gate.
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/CHANA/C0"), "/APP/OUTPUT_SELECT", 3);
  checkRegGatedBy(regs.getBackendRegister("/INHERIT/CHANA/C1"), "/APP/OUTPUT_SELECT", 3);
  // The channel slices of a 2D register with a register-level gate do NOT inherit it.
  checkRegGatedBy(regs.getBackendRegister("/REG2D_TOP/E0"), "", std::nullopt);
  checkRegGatedBy(regs.getBackendRegister("/REG2D_TOP/E1"), "", std::nullopt);
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

BOOST_AUTO_TEST_SUITE_END()
