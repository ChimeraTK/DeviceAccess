/**
 *      @file           DMapFileParser.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides method to parse DMAP file
 *                  
 */
#ifndef MTCA4U_DMAP_FILE_PARSER_H
#define MTCA4U_DMAP_FILE_PARSER_H

#include <stdint.h>
#include <string>
#include "DeviceInfoMap.h"

namespace ChimeraTK {

  /**
   * @brief  Provides method to parse DMAP file.
   *
   * Class does not perform parsing of associated MAP files.
   *
   */
  class DMapFileParser {
    public:
      /**
       * @brief Performs parsing of specified DMAP file. Returns pointer to
       * dRegisterInfo object describing all devices in file.
       *
       *
       * @throw exDmapFileParser [exLibMap::EX_DMAP_FILE_PARSE_ERROR] if parsing error
       * detected, exMapFileParser [exLibMap::EX_CANNOT_OPEN_DMAP_FILE] if specified DMAP file cannot be opened
       * or exMapFileParser [exLibMap::EX_NO_DATA_IN_DMAP_FILES] if specified DMAP file is empty
       * @param file_name name of DMAP file
       * @return pointer to dmapFile object
       */
      DeviceInfoMapPointer parse(const std::string &file_name);

    protected:
      // @todo FIXME passing the arguments C-style is super ugly
      void parseForLoadLib(std::string file_name, std::string line, uint32_t line_nr, DeviceInfoMapPointer dmap);
      void parseRegularLine(std::string file_name, std::string line, uint32_t line_nr, DeviceInfoMapPointer dmap);
      void raiseException(std::string file_name, std::string line, uint32_t line_nr);

      // Entries in the dmap file can be relative to the dmap file.
      // The dmap file itself can be an absolute or relative path.
      // This function return the absolute path of the dmap entry, correctly resolved with
      // respect to the dmap file.
      std::string absPathOfDMapContent(std::string dmapContent, std::string dmapFileName);

  };

}//namespace ChimeraTK

#endif  /* MTCA4U_DMAP_FILE_PARSER_H */

