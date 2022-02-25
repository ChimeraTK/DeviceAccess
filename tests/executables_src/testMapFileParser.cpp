#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE MapFileParser
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#  include <sstream>

#  include "Exception.h"
#  include "MapFileParser.h"
#  include "NumericAddressedRegisterCatalogue.h"
#  include "helperFunctions.h"

using namespace ChimeraTK;
using namespace boost::unit_test_framework;

BOOST_AUTO_TEST_SUITE(MapFileParserTestSuite)

/*******************************************************************************************************************/

void printRegInfo(const ChimeraTK::NumericAddressedRegisterInfo& info, std::ostream& ostr = std::cout) {
  ostr << "Register " << info.pathName << ": at " << info.bar << "," << info.address << " nElems = " << info.nElements
       << " pitch = " << info.elementPitchBits << " access: " << int(info.registerAccess)
       << " int: " << info.interruptCtrlNumber << "," << info.interruptNumber << std::endl;
  size_t ich = 0;
  for(const auto& c : info.channels) {
    ostr << "   channel " << ich << " at: " << c.bitOffset << " " << c.width << " " << c.nFractionalBits << " "
         << c.signedFlag << " " << int(c.dataType) << std::endl;
    ++ich;
  }
}

/*******************************************************************************************************************/

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

/*******************************************************************************************************************/
/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFileDoesNotExist) {
  ChimeraTK::MapFileParser fileparser;
  BOOST_CHECK_THROW(fileparser.parse("NonexistentFile.map"), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInvalidMetadata) {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("invalid_metadata.map"), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMandatoryRegisterFieldMissing) {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("MandatoryRegisterfIeldMissing.map"), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testIncorrectRegisterWidth) {
  ChimeraTK::MapFileParser map_file_parser;
  BOOST_CHECK_THROW(map_file_parser.parse("IncorrectRegisterWidth.map"), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFracBits) {
  ChimeraTK::MapFileParser map_file_parser1;
  ChimeraTK::MapFileParser map_file_parser2;
  BOOST_CHECK_THROW(map_file_parser1.parse("IncorrectFracBits1.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(map_file_parser2.parse("IncorrectFracBits2.map"), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

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

  compareCatalogue(regcat, RegisterInfoents);
}

/*******************************************************************************************************************/

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
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, 1, 3);
  RegisterInfoents[20] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_VOID2", 0x00, 0x0, 0x00, 0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, 1, 2);
  RegisterInfoents[21] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_TYPE", 0x01, 0x68, 0x04, 1, 18, 5,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 5, 6);

  compareCatalogue(regcat, RegisterInfoents);
}

/*******************************************************************************************************************/

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

/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInterruptBadMapFileParse) {
  ChimeraTK::MapFileParser fileparser;

  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError1.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError2.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError3.map"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(fileparser.parse("interruptMapFileWithError4.map"), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInterruptMapFileParse) {
  ChimeraTK::MapFileParser fileparser;
  auto [regcat, mdcat] = fileparser.parse("interruptMapFile.map");

  std::vector<ChimeraTK::NumericAddressedRegisterInfo> RegisterInfoents(11);

  RegisterInfoents[0] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_VOID_1", 0x00, 0x0, 0x00, 0x0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, 0, 0);

  RegisterInfoents[1] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_VOID_2", 0x00, 0x0, 0x00, 0x0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, 1, 1);

  RegisterInfoents[2] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_UINT_1", 0x01, 0x100, 0x04, 0x0, 32, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 2, 0);

  RegisterInfoents[3] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_INT_1", 0x01, 0x104, 0x04, 0x0, 32, 0,
      true, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 2, 1);

  RegisterInfoents[4] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_FIXPOINT_SIGNED", 0x01, 0x200, 0x04, 0x0, 32, 24, true,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 3, 0);

  RegisterInfoents[5] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_FIXPOINT_UNSIGNED", 0x01, 0x220, 0x04, 0x0, 32, 24, false,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 3, 1);

  RegisterInfoents[6] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_ARRAY_UINT", 0x03, 0x300, 12, 0x0, 32,
      0, false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 4, 0);

  RegisterInfoents[7] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_ARRAY_INT", 0x03, 0x400, 12, 0x0, 32, 0,
      true, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 4, 1);

  RegisterInfoents[8] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_ARRAY_FIXPOINT", 0x03, 0x500, 12, 0x0, 32, 24, false,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 4, 2);

  RegisterInfoents[9] = ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_AREA_INT", 0x0, 0x0, 0x05, 96,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false},
          {64, NumericAddressedRegisterInfo::Type::FIXED_POINT, 16, 0, false}},
      NumericAddressedRegisterInfo::Access::INTERRUPT, 5, 0);

  RegisterInfoents[10] =
      ChimeraTK::NumericAddressedRegisterInfo("APP0.INTERRUPT_AREA_INT.MULTIPLEXED_RAW", 0x0f, 0x0, 0x3c, 0x0, 32, 0,
          true, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 5, 0);

  compareCatalogue(regcat, RegisterInfoents);
}

/*******************************************************************************************************************/

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
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, 1, 3);
  RegisterInfoents[20] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_VOID2", 0x00, 0x0, 0x00, 0, 0, 0,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID, 1, 2);
  RegisterInfoents[21] = ChimeraTK::NumericAddressedRegisterInfo("MODULE0.INTERRUPT_TYPE", 0x01, 0x68, 0x04, 1, 18, 5,
      false, NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::FIXED_POINT, 5, 6);

  compareCatalogue(regcat, RegisterInfoents);
}

/*******************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
