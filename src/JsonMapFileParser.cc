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

    try {
      parseChildren(data.at("addressSpace"));
      return {std::move(_catalogue), std::move(_metadata)};
    }
    catch(json::out_of_range& e) {
      throw ChimeraTK::logic_error(std::format("Error parsing JSON map file '{}': {}", _fileName, e.what()));
    }
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void JsonMapFileParser::parseChildren(const json& data) {
    for(const auto& entry : data) {
      if(entry.contains("address")) {
        NumericAddressedRegisterInfo info;
        info.pathName = std::string(entry.at("name"));

        std::tie(info.bar, info.address) = parseAddress(entry.at("address"));

        _catalogue.addRegister(info);
      }

      if(entry.contains("children")) {
        parseChildren(entry.at("children"));
      }
    }
  }

  /********************************************************************************************************************/

  std::pair<uint64_t, uint64_t> JsonMapFileParser::parseAddress(const json& data) {
    uint64_t channel;

    std::string adrType = data.at("type");
    if(adrType == "I/O") {
      channel = data.at("channel");
    }
    else if(adrType == "DMA") {
      channel = 13UL + uint64_t(data.at("channel"));
    }
    else {
      throw ChimeraTK::logic_error(
          std::format("Error parsing JSON map file '{}': Address type '{}' is not supported.", _fileName, adrType));
    }

    return {channel, data.at("offset")};
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
