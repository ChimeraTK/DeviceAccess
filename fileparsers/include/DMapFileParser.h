// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceInfoMap.h"
#include "Exception.h"

#include <cstdint>
#include <string>

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
     * @throw exDmapFileParser [exLibMap::EX_DMAP_FILE_PARSE_ERROR] if parsing
     * error detected, exMapFileParser [exLibMap::EX_CANNOT_OPEN_DMAP_FILE] if
     * specified DMAP file cannot be opened or exMapFileParser
     * [exLibMap::EX_NO_DATA_IN_DMAP_FILES] if specified DMAP file is empty
     * @param file_name name of DMAP file
     * @return pointer to dmapFile object
     */
    DeviceInfoMapPointer parse(const std::string& file_name);

   protected:
    // @todo FIXME passing the arguments C-style is super ugly
    static void parseForLoadLib(
        const std::string& file_name, const std::string& line, uint32_t line_nr, const DeviceInfoMapPointer& dmap);
    static void parseRegularLine(
        const std::string& file_name, const std::string& line, uint32_t line_nr, const DeviceInfoMapPointer& dmap);
    static void raiseException(const std::string& file_name, const std::string& line, uint32_t line_nr);

    // Entries in the dmap file can be relative to the dmap file.
    // The dmap file itself can be an absolute or relative path.
    // This function return the absolute path of the dmap entry, correctly
    // resolved with respect to the dmap file.
    static std::string absPathOfDMapContent(const std::string& dmapContent, const std::string& dmapFileName);
  };

  namespace detail {

    /** This special exception is required only internally by the DMapFilesParser to
     * distinguish the special case of an empty DMAP file (in which case not the
     * entire parser should fail). */
    struct EmptyDMapFileException : ChimeraTK::logic_error {
      using ChimeraTK::logic_error::logic_error;
    };
  } // namespace detail

} // namespace ChimeraTK
