// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "MetadataCatalogue.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <string>

namespace ChimeraTK {

  class MapFileParser {
   public:
    /**
     * @brief Performs parsing of specified MAP file, resulting in catalogue objects describing all registers and
     * metadata available in file.
     *
     * @throw ChimeraTK::logic_error if parsing error detected or the specified MAP file cannot be opened.
     * @param fileName name of MAP file
     * @return pair of the register catalogue and the metadata catalogue
     */
    static std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> parse(const std::string& fileName);
  };

} // namespace ChimeraTK
