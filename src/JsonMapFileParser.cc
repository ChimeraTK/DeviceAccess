// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JsonMapFileParser.h"

#include <boost/algorithm/string.hpp>

#include <string>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  JsonMapFileParser::JsonMapFileParser(std::string fileName) : _fileName(std::move(fileName)) {}

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> JsonMapFileParser::parse(std::ifstream& stream) {
    // read and parse JSON data
    json data;
    try {
      data = json::parse(stream);
    }
    catch(const json::parse_error& e) {
      throw ChimeraTK::logic_error(std::format("Error parsing JSON map file '{}': {}", _fileName, e.what()));
    }

    requireKey(data, "addressSpace", "No address space defined.");

    NumericAddressedRegisterCatalogue catalogue;
    MetadataCatalogue metadata;

    for(const auto& entry : data["addressSpace"]) {
      requireKey(entry, "name", "Register entry does not contain a name.");

      NumericAddressedRegisterInfo info;
      info.pathName = std::string(entry["name"]);

      catalogue.addRegister(info);
    }

    return {std::move(catalogue), std::move(metadata)};
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void JsonMapFileParser::requireKey(const json& data, const std::string& key, const std::string& errorMessage) {
    if(!data.contains(key)) {
      throw ChimeraTK::logic_error(std::format("Error parsing JSON map file '{}': {}", _fileName, errorMessage));
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
