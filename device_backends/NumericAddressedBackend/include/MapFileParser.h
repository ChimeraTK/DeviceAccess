// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Exception.h"
#include "MetadataCatalogue.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <list>
#include <string>

namespace ChimeraTK {

  /**
   * @brief  Provides method to parse MAP file
   *
   */
  class MapFileParser {
   public:
    /**
     * @brief Performs parsing of specified MAP file. Returns pointer to
     * RegisterInfo object describing all registers and metadata available in
     * file.
     *
     *
     * @throw ChimeraTK::logic_error if parsing error detected or the specified MAP
     * file cannot be opened
     * @param file_name name of MAP file
     * @return pointer to RegisterInfoMap object
     *
     *
     */
    std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> parse(const std::string& file_name);

   private:
    /** Hold parsed content of a single line */
    struct ParsedLine {
      RegisterPath pathName;      /**< Name of register */
      uint32_t nElements{0};      /**< Number of elements in register */
      uint64_t address{0};        /**< Relative address in bytes from beginning of the bar (Base Address Range) */
      uint32_t nBytes{0};         /**< Size of register expressed in bytes */
      uint64_t bar{0};            /**< Number of bar with register */
      uint32_t width{32};         /**< Number of significant bits in the register */
      int32_t nFractionalBits{0}; /**< Number of fractional bits */
      bool signedFlag{true};      /**< Signed/Unsigned flag */
      NumericAddressedRegisterInfo::Access registerAccess{NumericAddressedRegisterInfo::Access::READ_WRITE};
      NumericAddressedRegisterInfo::Type type{NumericAddressedRegisterInfo::Type::FIXED_POINT};
      std::vector<uint32_t> interruptID;
    };

    /**
     * Split the string at the last dot. The part up to the last dot is the first
     * returned argument, the part after the last dot is the second. Hence, the
     * first part can contain dots itself, the second part cannot. If there is no
     * dot, the first part is empty and the full string is returned as second (the
     * part up to the first dot is considered as prefix).
     */
    static std::pair<RegisterPath, std::string> splitStringAtLastDot(RegisterPath moduleDotName);

    static std::pair<NumericAddressedRegisterInfo::Type, int> getTypeAndNFractionalBits(
        const std::string& bitInterpretation, uint32_t width);

    // returns an empty vector if the type is not INTERRUPT
    static std::vector<uint32_t> getInterruptId(std::string accessType);

    static void checkFileConsitencyAndThrowIfError(NumericAddressedRegisterInfo::Access registerAccessMode,
        NumericAddressedRegisterInfo::Type registerType, uint32_t nElements, uint64_t address, uint32_t nBytes,
        uint64_t bar, uint32_t width, int32_t nFractionalBits, bool signedFlag);

    void parseMetaData(std::string line);

    ParsedLine parseLine(const std::string& line);

    /**
     * On detection of a AREA_MULTIPLEXED_SEQUENCE line, collects the associated paresed lines and creates the
     * according RegisterInfo instance(s)
     */
    void handle2D(const ParsedLine& pl);

    /**
     * On detection of line with a MEM_MULTIPLEXED 2D declaration collects the associated paresed lines and
     * creates the according RegisterInfo instance(s)
     */
    void handle2DNewStyle(const ParsedLine& pl);

    /**
     * Checks whether the register name does not contain the special prefixes marking multiplexed registers and
     * sequences etc.
     */
    static bool isScalarOr1D(const RegisterPath& pathName);

    /**
     * Checks whether the register name contains the prefix for a multiplexed register (but not for the individual
     * sequences, so only the "main" entry matches).
     */
    static bool is2D(const RegisterPath& pathName);

    /**
     * Checks whether the register name contains the prefix for a multiplexed register (but not for the individual
     * sequences, so only the "main" entry matches).
     */
    static bool is2DNewStyle(RegisterPath pathName);

    /**
     * Generate sequence name from main entry for multiplexed registers
     */
    static RegisterPath makeSequenceName(const RegisterPath& pathName, size_t index);

    /**
     * Generate 2D register name from main entry for multiplexed registers
     */
    static RegisterPath make2DName(const RegisterPath& pathName, const std::string& prefix);

    /**
     * Creates the two RegisterInfos that belong to a 2D multiplexed area, with a prefix according to the old or
     * new syntax
     */
    void make2DRegisterInfos(const ParsedLine& pl, std::list<ParsedLine>& channelLines, const std::string& prefix);

    NumericAddressedRegisterCatalogue pmap;
    MetadataCatalogue metadataCatalogue;

    std::string file_name;
    uint32_t line_nr = 0;

    std::vector<ParsedLine> parsedLines;
    std::map<RegisterPath, const ParsedLine&> parsedLinesMap;
  };

} // namespace ChimeraTK
