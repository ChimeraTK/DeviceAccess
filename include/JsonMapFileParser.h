// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "MetadataCatalogue.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <fstream>

namespace ChimeraTK::detail {

  /**
   * @brief  Provides method to parse MAP file
   *
   */
  class JsonMapFileParser {
   public:
    explicit JsonMapFileParser(std::string fileName);
    ~JsonMapFileParser();

    /**
     * @brief Performs parsing of given input stream.
     *
     * @throw ChimeraTK::logic_error if parsing error detected
     * @param stream input stream to process
     * @return pair of the register catalogue and the metadata catalogue
     */
    std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> parse(std::ifstream& stream);

   private:
    struct Imp;
    std::unique_ptr<Imp> _theImp;
  };

} // namespace ChimeraTK::detail
