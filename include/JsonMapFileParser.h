// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "MetadataCatalogue.h"
#include "NumericAddressedRegisterCatalogue.h"
#include <nlohmann/json.hpp>

#include <fstream>

namespace ChimeraTK::detail {

  /**
   * @brief  Provides method to parse MAP file
   *
   */
  class JsonMapFileParser {
    using json = nlohmann::json;

   public:
    explicit JsonMapFileParser(std::string fileName);

    /**
     * @brief Performs parsing of given input stream.
     *
     * @throw ChimeraTK::logic_error if parsing error detected
     * @param stream input stream to process
     * @return pair of the register catalogue and the metadata catalogue
     */
    std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> parse(std::ifstream& stream);

   private:
    std::string _fileName;
    NumericAddressedRegisterCatalogue _catalogue;
    MetadataCatalogue _metadata;

    /// Recursive parsing
    void parseChildren(const RegisterPath& namePrefix, const json& data);

    /// Returns channel and offset as a pair
    std::pair<uint64_t, uint64_t> parseAddress(const json& data);
  };

} // namespace ChimeraTK::detail
