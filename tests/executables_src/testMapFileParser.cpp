// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE MapFileParser
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Exception.h"
#include "helperFunctions.h"
#include "MapFileParser.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <sstream>

using namespace ChimeraTK;
using namespace boost::unit_test_framework;

BOOST_AUTO_TEST_SUITE(MapFileParserTestSuite)

/**********************************************************************************************************************/

void printRegInfo(const ChimeraTK::NumericAddressedRegisterInfo& info, std::ostream& ostr = std::cout) {
  ostr << "Register " << info.pathName << ": at " << info.bar << "," << info.address << " nElems = " << info.nElements
       << " pitch = " << info.elementPitchBits << " access: " << int(info.registerAccess) << " int: [";
  for(auto i : info.interruptId) {
    ostr << i << ",";
  }
  ostr << "]" << std::endl;
  size_t ich = 0;
  for(const auto& c : info.channels) {
    ostr << "   channel " << ich << " at: " << c.bitOffset << " " << c.width << " " << c.nFractionalBits << " "
         << c.signedFlag << " " << int(c.dataType) << std::endl;
    ++ich;
  }
}

/**********************************************************************************************************************/

void compareCatalogue(const NumericAddressedRegisterCatalogue& regcat,
    const std::vector<ChimeraTK::NumericAddressedRegisterInfo>& RegisterInfoents) {
  // first check that size is equal
  BOOST_CHECK_EQUAL(regcat.getNumberOfRegisters(), RegisterInfoents.size());

  // compare and check for same order of elements
  auto mapIter = regcat.begin();
  auto elementsIter = RegisterInfoents.begin();
  for(; mapIter != regcat.end() && elementsIter != RegisterInfoents.end(); ++mapIter, ++elementsIter) {
    std::stringstream message;
    message << "Failed comparison:" << std::endl;
    printRegInfo(*mapIter, message);
    printRegInfo(*elementsIter, message);
    BOOST_CHECK_MESSAGE(*mapIter == *elementsIter, message.str());
  }
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFileDoesNotExist) {
  ChimeraTK::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("NonexistentFile.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInvalidMetadata) {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("invalid_metadata.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMandatoryRegisterFieldMissing) {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("MandatoryRegisterfIeldMissing.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testIncorrectRegisterWidth) {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth2.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth3.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth4.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFracBits) {
  ChimeraTK::MapFileParser map_file_parser1;
  ChimeraTK::MapFileParser map_file_parser2;
  BOOST_CHECK_THROW(map_file_parser1.parse("IncorrectFracBits1.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(map_file_parser2.parse("IncorrectFracBits2.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(test64BitSequence) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("64BitSequence.map");

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents;
  RegisterInfoents.emplace_back(ChimeraTK::NumericAddressedRegisterInfo("INT642D", 0x0, 0x0, 0x02, 192,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 64, 0, false, DataType::int32},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 64, 0, false, DataType::int32},
          {128, NumericAddressedRegisterInfo::Type::FIXED_POINT, 64, 0, false, DataType::int32}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {}));

  RegisterInfoents.emplace_back(
      ChimeraTK::NumericAddressedRegisterInfo("INT642D.MULTIPLEXED_RAW", 0x06, 0x0, 0x30, 0x0, 64, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {}));

  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testGoodMapFileParse) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("goodMapFile_withoutModules.map");

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  retrievedValue = mdcat.getMetadata(metaDataNameToRetrieve);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  retrievedValue = mdcat.getMetadata(metaDataNameToRetrieve);
  BOOST_CHECK(retrievedValue == "2.5");

  /* TODO: remove default assignments to unused fields in the parse
   * function and move it all to the constructor */

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents;
  RegisterInfoents.emplace_back("WORD_FIRMWARE", 0x00000001, 0x00000000, 0x00000004, 0x0, 32, 0, true);
  RegisterInfoents.emplace_back("WORD_COMPILATION", 0x00000001, 0x00000004, 0x00000004, 0x00000000, 32, 0, true);
  RegisterInfoents.emplace_back("WORD_STATUS", 0x00000001, 0x00000008, 0x00000004, 0x00000000, 32, 0, true);
  RegisterInfoents.emplace_back("WORD_USER1", 0x00000001, 0x0000000C, 0x00000004, 0x00000000, 32, 0, true);
  RegisterInfoents.emplace_back("WORD_USER2", 0x00000001, 0x00000010, 0x00000004, 0x00000000, 32, 0, false);

  RegisterInfoents.emplace_back(ChimeraTK::NumericAddressedRegisterInfo("INT2D", 0x0, 0x0, 0x05, 96,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false, DataType::int32},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false, DataType::int32},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false, DataType::int32}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {}));

  RegisterInfoents.emplace_back(
      ChimeraTK::NumericAddressedRegisterInfo("INT2D.MULTIPLEXED_RAW", 0x0f, 0x0, 0x3c, 0x0, 32, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {}));
  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test64BitScalar) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("64BitScalar.map");
  BOOST_CHECK_EQUAL(regcat.getNumberOfRegisters(), 1);
  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(1);
  RegisterInfoents[0] =
      ChimeraTK::NumericAddressedRegisterInfo("WORD_64BitScalar", 0x01, 0x00, 0x08, 0x00, 64, 0, false);

  compareCatalogue(regcat, RegisterInfoents);
}
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testGoodMappFileParse) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("goodMapFile.map");

  BOOST_CHECK_EQUAL(regcat.getNumberOfRegisters(), 22);

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  retrievedValue = mdcat.getMetadata(metaDataNameToRetrieve);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  retrievedValue = mdcat.getMetadata(metaDataNameToRetrieve);
  BOOST_CHECK(retrievedValue == "2.5");

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(22);

  RegisterInfoents[0] =
      ChimeraTK::NumericAddressedRegisterInfo("BOARD.WORD_FIRMWARE", 0x01, 0x0, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[1] =
      ChimeraTK::NumericAddressedRegisterInfo("BOARD.WORD_COMPILATION", 0x01, 0x04, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[2] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.WORD_STATUS", 0x01, 0x08, 0x04, 0x01, 32, 0, true);
  RegisterInfoents[3] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.WORD_SCRATCH", 0x01, 0x08, 0x04, 0x01, 16, 0, true);
  RegisterInfoents[4] = ChimeraTK::NumericAddressedRegisterInfo("APP0.MODULE0", 0x03, 0x10, 0x0C, 0x01, 32, 0, true);
  RegisterInfoents[5] = ChimeraTK::NumericAddressedRegisterInfo("APP0.MODULE1", 0x03, 0x20, 0x0C, 0x01, 32, 0, true);
  RegisterInfoents[6] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE0.WORD_USER1", 0x01, 0x10, 0x04, 0x01, 16, 3, true);
  RegisterInfoents[7] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE0.WORD_USER2", 0x01, 0x14, 0x04, 0x01, 18, 5, false);
  RegisterInfoents[8] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE0.WORD_USER3", 0x01, 0x18, 0x04, 0x01, 18, 5, false);
  RegisterInfoents[9] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE1.WORD_USER1", 0x01, 0x20, 0x04, 0x01, 16, 3, true);
  RegisterInfoents[10] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE1.WORD_USER2", 0x01, 0x24, 0x04, 0x01, 18, 5, false);
  RegisterInfoents[11] = ChimeraTK::NumericAddressedRegisterInfo(
      "MODULE1.WORD_USER3", 0x01, 0x28, 0x04, 0x01, 18, 5, false, NumericAddressedRegisterInfo::Access::READ_ONLY);
  RegisterInfoents[12] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE2.NO_OPTIONAL", 0x01, 0x2C, 0x04, 0x01, 32, 0, true);
  RegisterInfoents[13] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE.NAME.WITH.DOTS.REGISTER", 0x01, 0x00, 0x04, 0x02, 32, 0, true);
  RegisterInfoents[14] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE1.TEST_AREA", 0x0A, 0x025, 0x028, 0x01, 32, 0, false);
  RegisterInfoents[15] = ChimeraTK::NumericAddressedRegisterInfo("FLOAT_TEST.SCALAR", 0x01, 0x060, 0x04, 0x01, 32, 0,
      true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::IEEE754);
  RegisterInfoents[16] = ChimeraTK::NumericAddressedRegisterInfo("FLOAT_TEST.ARRAY", 0x04, 0x064, 0x010, 0x01, 32, 0,
      true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::IEEE754);
  RegisterInfoents[17] =
      ChimeraTK::NumericAddressedRegisterInfo("BOARD.NO_OPTIONAL", 0x01, 0x08, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[18] =
      ChimeraTK::NumericAddressedRegisterInfo("LARGE_BAR.NUMBER", 0x01, 0x0, 0x04, 0x100000000, 32, 0, true);
  RegisterInfoents[19] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_VOID1", 0x00, 0x0, 0x00, 0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {3});
  RegisterInfoents[20] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_VOID2", 0x00, 0x0, 0x00, 0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {2});
  RegisterInfoents[21] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_TYPE", 0x01, 0x68, 0x04, 1, 18, 5,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {6});

  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMixedMapFileParse) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("mixedMapFile.map");

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(4);

  RegisterInfoents[0] = ChimeraTK::NumericAddressedRegisterInfo("WORD_FIRMWARE_ID", 0x01, 0x0, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[1] = ChimeraTK::NumericAddressedRegisterInfo("WORD_USER", 0x01, 0x4, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[2] = ChimeraTK::NumericAddressedRegisterInfo("APP0.MODULE_ID", 0x01, 0x0, 0x04, 0x1, 32, 0, true);
  RegisterInfoents[3] = ChimeraTK::NumericAddressedRegisterInfo("APP0.WORD_USER", 0x03, 0x4, 0x0C, 0x1, 18, 3, false);

  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInterruptBadMapFileParse) {
  ChimeraTK::MapFileParser fileparser;

  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError1.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError2.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError3.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError4.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInterruptMapFileParse) {
  ChimeraTK::MapFileParser fileparser;
  auto [regcat, mdcat] = fileparser.parse("interruptMapFile.map");

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(13);

  RegisterInfoents[0] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_VOID_1", 0x00, 0x0, 0x00, 0x0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {0});

  RegisterInfoents[1] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_VOID_2", 0x00, 0x0, 0x00, 0x0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {1, 1});

  RegisterInfoents[2] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_VOID_3", 0x00, 0x0, 0x00, 0x0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {2, 2, 2});

  RegisterInfoents[3] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_UINT_1", 0x01, 0x100, 0x04, 0x0, 32, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {20});

  RegisterInfoents[4] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.SUB_INTERRUPT_INT_1", 0x01, 0x104, 0x04, 0x0, 32, 0, true,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {21, 1});

  RegisterInfoents[5] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.SUB_SUB_INTERRUPT_INT_2", 0x01, 0x108, 0x04, 0x0, 32, 0, true,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {22, 3, 4});

  RegisterInfoents[6] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_FIXPOINT_SIGNED", 0x01, 0x200, 0x04, 0x0, 32, 24, true,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {3});

  RegisterInfoents[7] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_FIXPOINT_UNSIGNED", 0x01, 0x220, 0x04, 0x0, 32, 24, false,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {3});

  RegisterInfoents[8] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_ARRAY_UINT", 0x03, 0x300, 12, 0x0, 32,
      0, false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {4});

  RegisterInfoents[9] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_ARRAY_INT", 0x03, 0x400, 12, 0x0, 32, 0,
      true, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {4});

  RegisterInfoents[10] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_ARRAY_FIXPOINT", 0x03, 0x500, 12, 0x0, 32, 24, false,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {4});

  RegisterInfoents[11] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_AREA_INT", 0x0, 0x0, 0x05, 96,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false, DataType::int32},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false, DataType::int32},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false, DataType::int32}},
      NumericAddressedRegisterInfo::Access::INTERRUPT, {5});

  RegisterInfoents[12] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_AREA_INT.MULTIPLEXED_RAW", 0x0f, 0x0, 0x3c, 0x0, 32, 0,
          true, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {5});

  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMapFileWithCommentsParse) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("goodMapFileWithComments.map");

  BOOST_CHECK_EQUAL(regcat.getNumberOfRegisters(), 22);

  std::string metaDataNameToRetrieve;
  std::string retrievedValue;

  metaDataNameToRetrieve = "HW_VERSION";
  retrievedValue = mdcat.getMetadata(metaDataNameToRetrieve);
  BOOST_CHECK(retrievedValue == "1.6");

  metaDataNameToRetrieve = "FW_VERSION";
  retrievedValue = mdcat.getMetadata(metaDataNameToRetrieve);
  BOOST_CHECK(retrievedValue == "2.5");

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(22);

  RegisterInfoents[0] =
      ChimeraTK::NumericAddressedRegisterInfo("BOARD.WORD_FIRMWARE", 0x01, 0x0, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[1] =
      ChimeraTK::NumericAddressedRegisterInfo("BOARD.WORD_COMPILATION", 0x01, 0x04, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[2] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.WORD_STATUS", 0x01, 0x08, 0x04, 0x01, 32, 0, true);
  RegisterInfoents[3] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.WORD_SCRATCH", 0x01, 0x08, 0x04, 0x01, 16, 0, true);
  RegisterInfoents[4] = ChimeraTK::NumericAddressedRegisterInfo("APP0.MODULE0", 0x03, 0x10, 0x0C, 0x01, 32, 0, true);
  RegisterInfoents[5] = ChimeraTK::NumericAddressedRegisterInfo("APP0.MODULE1", 0x03, 0x20, 0x0C, 0x01, 32, 0, true);
  RegisterInfoents[6] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE0.WORD_USER1", 0x01, 0x10, 0x04, 0x01, 16, 3, true);
  RegisterInfoents[7] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE0.WORD_USER2", 0x01, 0x14, 0x04, 0x01, 18, 5, false);
  RegisterInfoents[8] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE0.WORD_USER3", 0x01, 0x18, 0x04, 0x01, 18, 5, false);
  RegisterInfoents[9] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE1.WORD_USER1", 0x01, 0x20, 0x04, 0x01, 16, 3, true);
  RegisterInfoents[10] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE1.WORD_USER2", 0x01, 0x24, 0x04, 0x01, 18, 5, false);
  RegisterInfoents[11] = ChimeraTK::NumericAddressedRegisterInfo(
      "MODULE1.WORD_USER3", 0x01, 0x28, 0x04, 0x01, 18, 5, false, NumericAddressedRegisterInfo::Access::READ_ONLY);
  RegisterInfoents[12] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE2.NO_OPTIONAL", 0x01, 0x2C, 0x04, 0x01, 32, 0, true);
  RegisterInfoents[13] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE.NAME.WITH.DOTS.REGISTER", 0x01, 0x00, 0x04, 0x02, 32, 0, true);
  RegisterInfoents[14] =
      ChimeraTK::NumericAddressedRegisterInfo("MODULE1.TEST_AREA", 0x0A, 0x025, 0x028, 0x01, 32, 0, false);
  RegisterInfoents[15] = ChimeraTK::NumericAddressedRegisterInfo("FLOAT_TEST.SCALAR", 0x01, 0x060, 0x04, 0x01, 32, 0,
      true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::IEEE754);
  RegisterInfoents[16] = ChimeraTK::NumericAddressedRegisterInfo("FLOAT_TEST.ARRAY", 0x04, 0x064, 0x010, 0x01, 32, 0,
      true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::IEEE754);
  RegisterInfoents[17] =
      ChimeraTK::NumericAddressedRegisterInfo("BOARD.NO_OPTIONAL", 0x01, 0x08, 0x04, 0x0, 32, 0, true);
  RegisterInfoents[18] =
      ChimeraTK::NumericAddressedRegisterInfo("LARGE_BAR.NUMBER", 0x01, 0x0, 0x04, 0x100000000, 32, 0, true);
  RegisterInfoents[19] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_VOID1", 0x00, 0x0, 0x00, 0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {1, 3});
  RegisterInfoents[20] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_VOID2", 0x00, 0x0, 0x00, 0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, {1, 2});
  RegisterInfoents[21] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_TYPE", 0x01, 0x68, 0x04, 1, 18, 5,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, {5, 6});

  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMapFileNewStyleMuxed) {
  ChimeraTK::MapFileParser map_file_parser;
  auto [regcat, mdcat] = map_file_parser.parse("newSequences.mapp");

  BOOST_CHECK_EQUAL(regcat.getNumberOfRegisters(), 18);

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(18);
  RegisterInfoents[0] = ChimeraTK::NumericAddressedRegisterInfo("TEST.INT", 0x0, 0x0, 0x05, 96,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("uint32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("uint32")},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[1] = ChimeraTK::NumericAddressedRegisterInfo("TEST.INT.MULTIPLEXED_RAW", 0x0f, 0x0, 0x3c, 0x0, 32, 0,
      true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[2] = ChimeraTK::NumericAddressedRegisterInfo("TEST.CHAR", 0x0, 0x40, 0x05, 24,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {8, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {16, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[3] = ChimeraTK::NumericAddressedRegisterInfo("TEST.CHAR.MULTIPLEXED_RAW", 0x04, 0x40, 0x10, 0x0, 32,
      0, true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[4] = ChimeraTK::NumericAddressedRegisterInfo("TEST.SHORT", 0x0, 0x50, 0x05, 48,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {16, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[5] = ChimeraTK::NumericAddressedRegisterInfo("TEST.SHORT.MULTIPLEXED_RAW", 0x08, 0x50, 0x20, 0x0, 32,
      0, true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[6] = ChimeraTK::NumericAddressedRegisterInfo("TEST.FRAC_INT", 0x1, 0x0, 0x05, 96,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 1, true, ChimeraTK::DataType("uint32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 2, true, ChimeraTK::DataType("uint32")},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 3, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[7] =
      ChimeraTK::NumericAddressedRegisterInfo("TEST.FRAC_INT.MULTIPLEXED_RAW", 0x0f, 0x0, 0x3c, 0x01, 32, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[8] = ChimeraTK::NumericAddressedRegisterInfo("TEST.FRAC_CHAR", 0x1, 0x40, 0x05, 24,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 1, true, ChimeraTK::DataType("uint32")},
          {8, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 2, true, ChimeraTK::DataType("uint32")},
          {16, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 3, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[9] =
      ChimeraTK::NumericAddressedRegisterInfo("TEST.FRAC_CHAR.MULTIPLEXED_RAW", 0x04, 0x40, 0x10, 0x01, 32, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[10] = ChimeraTK::NumericAddressedRegisterInfo("TEST.FRAC_SHORT", 0x1, 0x50, 0x05, 48,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 1, true, ChimeraTK::DataType("uint32")},
          {16, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 2, true, ChimeraTK::DataType("uint32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 3, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[11] =
      ChimeraTK::NumericAddressedRegisterInfo("TEST.FRAC_SHORT.MULTIPLEXED_RAW", 0x08, 0x50, 0x20, 0x01, 32, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[12] = ChimeraTK::NumericAddressedRegisterInfo("TEST.DMA", 0x0d, 0x0, 0x04, 256,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {16, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {48, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {80, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {96, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {112, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {128, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {144, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {160, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {176, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {192, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {208, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {224, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")},
          {240, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, -2, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[13] = ChimeraTK::NumericAddressedRegisterInfo("TEST.DMA.MULTIPLEXED_RAW", 0x20, 0x00, 0x80, 0x0d, 32,
      0, true, NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[14] = ChimeraTK::NumericAddressedRegisterInfo("TEST.MIXED", 0x3, 0x00, 0x03, 120,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {8, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {24, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("uint32")},
          {56, NumericAddressedRegisterInfo::Type::FIXED_POINT, 64, 0, true, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[15] =
      ChimeraTK::NumericAddressedRegisterInfo("TEST.MIXED.MULTIPLEXED_RAW", 0x06, 0x00, 0x30, 0x03, 64, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  RegisterInfoents[16] = ChimeraTK::NumericAddressedRegisterInfo("APP0.DAQ0_BAM", 0x02, 0x0, 372, 352,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {16, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 18, 0, true, ChimeraTK::DataType("uint32")},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {80, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, true, ChimeraTK::DataType("uint32")},
          {96, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {112, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {128, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {136, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {144, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {152, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {160, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {192, NumericAddressedRegisterInfo::Type::FIXED_POINT, 8, 0, true, ChimeraTK::DataType("uint32")},
          {224, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("uint32")},
          {256, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, false, ChimeraTK::DataType("uint32")},
          {288, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, false, ChimeraTK::DataType("uint32")},
          {320, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, false, ChimeraTK::DataType("uint32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, {});

  RegisterInfoents[17] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.DAQ0_BAM.MULTIPLEXED_RAW", 0x1000, 0x00, 0x4000, 0x02, 32, 0, true,
          NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, {});

  compareCatalogue(regcat, RegisterInfoents);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
