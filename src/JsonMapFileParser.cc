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
      parseChildren("/", data.at("addressSpace"));
      return {std::move(_catalogue), std::move(_metadata)};
    }
    catch(json::out_of_range& e) {
      throw ChimeraTK::logic_error(std::format("Error parsing JSON map file '{}': {}", _fileName, e.what()));
    }
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  void JsonMapFileParser::parseChildren(const RegisterPath& namePrefix, const json& data) {
    for(const auto& entry : data) {
      RegisterPath name = namePrefix / entry.at("name");
      if(entry.contains("address")) {
        NumericAddressedRegisterInfo info;
        info.pathName = name;

        std::tie(info.bar, info.address) = parseAddress(entry["address"]);

        if(entry.contains("access") && entry.contains("triggeredByInterrupt")) {
        }
        std::string access = entry.at("access");
        if(access == "RW") {
          info.registerAccess = NumericAddressedRegisterInfo::Access::READ_WRITE;
        }
        else if(access == "RO") {
          info.registerAccess = NumericAddressedRegisterInfo::Access::READ_ONLY;
        }
        else if(access == "WO") {
          info.registerAccess = NumericAddressedRegisterInfo::Access::WRITE_ONLY;
        }
        else {
          throw ChimeraTK::logic_error(
              std::format("Error parsing JSON map file '{}': Illegal access type '{}'", _fileName, access));
        }

        if(entry.contains("triggeredByInterrupt")) {
          info.flags.add(AccessMode::wait_for_new_data);
        }

        _catalogue.addRegister(info);
      }

      if(entry.contains("children")) {
        parseChildren(name, entry["children"]);
      }
    }
  }

  /********************************************************************************************************************/

  std::pair<uint64_t, uint64_t> JsonMapFileParser::parseAddress(const json& data) {
    uint64_t channel;

    std::string adrType = data.value("type", "I/O");
    if(adrType == "I/O") {
      channel = data.value("channel", 0);
    }
    else if(adrType == "DMA") {
      channel = 13UL + uint64_t(data.value("channel", 0));
    }
    else {
      throw ChimeraTK::logic_error(
          std::format("Error parsing JSON map file '{}': Address type '{}' is not supported.", _fileName, adrType));
    }

    return {channel, data.at("offset")};
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
