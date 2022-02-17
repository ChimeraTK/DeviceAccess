#ifndef CHIMERA_TK_MAPFILEPARSER_H
#define CHIMERA_TK_MAPFILEPARSER_H

#include "Exception.h"
#include "RegisterInfoMap.h"
#include "MetadataCatalogue.h"

#include <fstream>
#include <iomanip>
#include <cstdint>
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
     * @throw exMapFileParser [LibMapException::EX_MAP_FILE_PARSE_ERROR] if
     * parsing error detected or exMapFileParser
     * [LibMapException::EX_CANNOT_OPEN_MAP_FILE] if specified MAP file cannot be
     * opened
     * @param file_name name of MAP file
     * @return pointer to RegisterInfoMap object
     *
     *
     */
    std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> parse(const std::string& file_name);

    /** Split the string at the last dot. The part up to the last dot is the first
     * returned argument, the part after the last dot is the second. Hence, the
     * first part can contain dots itself, the second part cannot. If there is no
     * dot, the first part is empty and the full string is returned as second (the
     * part up to the first dot is considered as prefix).
     */
    static std::pair<std::string, std::string> splitStringAtLastDot(std::string moduleDotName);

   protected:
    std::pair<NumericAddressedRegisterInfo::Type, int> getTypeAndNFractionalBits(
        std::string bitInterpretation, uint32_t width);

    std::pair<bool, std::pair<unsigned int, unsigned int>> getInterruptData(std::string accessType);

    void checkFileConsitencyAndThrowIfError(NumericAddressedRegisterInfo::Access registerAccessMode,
        NumericAddressedRegisterInfo::Type registerType, uint32_t nElements, uint64_t address, uint32_t nBytes,
        uint64_t bar, uint32_t width, int32_t nFractionalBits, bool signedFlag);
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_MAPFILEPARSER_H */
