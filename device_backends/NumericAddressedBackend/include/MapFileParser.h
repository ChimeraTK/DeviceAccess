#pragma once

#include "NumericAddressedRegisterCatalogue.h"
#include "Exception.h"
#include "MetadataCatalogue.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <string>
#include <list>

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
      uint32_t interruptCtrlNumber{0};
      uint32_t interruptNumber{0};
    };

    /** 
     * Split the string at the last dot. The part up to the last dot is the first
     * returned argument, the part after the last dot is the second. Hence, the
     * first part can contain dots itself, the second part cannot. If there is no
     * dot, the first part is empty and the full string is returned as second (the
     * part up to the first dot is considered as prefix).
     */
    std::pair<RegisterPath, std::string> splitStringAtLastDot(RegisterPath moduleDotName);

    std::pair<NumericAddressedRegisterInfo::Type, int> getTypeAndNFractionalBits(
        std::string bitInterpretation, uint32_t width);

    std::pair<bool, std::pair<unsigned int, unsigned int>> getInterruptData(std::string accessType);

    void checkFileConsitencyAndThrowIfError(NumericAddressedRegisterInfo::Access registerAccessMode,
        NumericAddressedRegisterInfo::Type registerType, uint32_t nElements, uint64_t address, uint32_t nBytes,
        uint64_t bar, uint32_t width, int32_t nFractionalBits, bool signedFlag);

    void parseMetaData(std::string line);

    ParsedLine parseLine(std::string line);

    void handle2D(const ParsedLine& pl);

    void handle2DNewStyle(const ParsedLine& pl);

    /** 
     * Checks whether the register name does not contain the special prefixes marking multiplexed registers and
     * sequences etc.
     */
    bool isScalarOr1D(const RegisterPath& pathName);

    /**
     * Checks whether the register name contains the prefix for a multiplexed register (but not for the individual
     * sequences, so only the "main" entry matches).
     */
    bool is2D(const RegisterPath& pathName);

    /**
     * Checks whether the register name contains the prefix for a multiplexed register (but not for the individual
     * sequences, so only the "main" entry matches).
     */
    bool is2DNewStyle(RegisterPath pathName);

    /**
     * Generate sequence name from main entry for multiplexed registers
     */
    RegisterPath makeSequenceName(const RegisterPath& pathName, size_t index);

    /**
     * Generate 2D register name from main entry for multiplexed registers
     */
    RegisterPath make2DName(const RegisterPath& pathName, const std::string& prefix);

    void createMuxedAccessors(const ParsedLine &pl, std::list<ParsedLine>& channelLines, const std::string& prefix);

    NumericAddressedRegisterCatalogue pmap;
    MetadataCatalogue metadataCatalogue;

    std::string file_name;
    uint32_t line_nr = 0;

    std::vector<ParsedLine> parsedLines;
    std::map<RegisterPath, const ParsedLine&> parsedLinesMap;
  };

} // namespace ChimeraTK
