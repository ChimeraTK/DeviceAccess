// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JsonMapFileParser.h"

#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>

#include <string>

using json = nlohmann::json;

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  struct JsonAddressSpaceEntry;

  struct JsonMapFileParser::Imp {
    std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> parse(std::ifstream& stream);

    std::string fileName;
    NumericAddressedRegisterCatalogue catalogue;
    MetadataCatalogue metadata;
  };

  /********************************************************************************************************************/

  JsonMapFileParser::JsonMapFileParser(std::string fileName) : _theImp(std::make_unique<Imp>(std::move(fileName))) {}

  JsonMapFileParser::~JsonMapFileParser() = default;

  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> JsonMapFileParser::parse(std::ifstream& stream) {
    return _theImp->parse(stream);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  // map Access enum to JSON as strings. Need to redefine the strongly typed enums as old-fashioned ones....
  enum Access {
    READ_ONLY = int(NumericAddressedRegisterInfo::Access::READ_ONLY),
    WRITE_ONLY = int(NumericAddressedRegisterInfo::Access::WRITE_ONLY),
    READ_WRITE = int(NumericAddressedRegisterInfo::Access::READ_WRITE),
    accessNotSet
  };
  NLOHMANN_JSON_SERIALIZE_ENUM(
      Access, {{Access::READ_ONLY, "RO"}, {Access::READ_WRITE, "RW"}, {Access::WRITE_ONLY, "WO"}})

  /********************************************************************************************************************/

  // map RepresentationType enum to JSON as strings
  enum RepresentationType {
    VOID = int(NumericAddressedRegisterInfo::Type::VOID),
    FIXED_POINT = int(NumericAddressedRegisterInfo::Type::FIXED_POINT),
    IEEE754 = int(NumericAddressedRegisterInfo::Type::IEEE754),
    ASCII = int(NumericAddressedRegisterInfo::Type::ASCII),
    representationNotSet
  };
  NLOHMANN_JSON_SERIALIZE_ENUM(RepresentationType,
      {{RepresentationType::FIXED_POINT, "fixedPoint"}, {RepresentationType::IEEE754, "IEEE754"},
          {RepresentationType::VOID, "void"}, {RepresentationType::ASCII, "string"}})

  /********************************************************************************************************************/

  // map AddressType enum to JSON as strings
  enum AddressType { IO, DMA, addressTypeNotSet };
  NLOHMANN_JSON_SERIALIZE_ENUM(AddressType, {{AddressType::IO, "IO"}, {AddressType::DMA, "DMA"}})

  /********************************************************************************************************************/

  // Allow hex string representation of values (but still accept plain int as well)
  struct HexValue {
    size_t v;

    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void from_json(const json& j, HexValue& hv) {
      if(j.is_string()) {
        auto sdata = std::string(j);
        try {
          hv.v = std::stoll(sdata, nullptr, 0);
        }
        catch(std::invalid_argument& e) {
          throw json::type_error::create(0, "Cannot parse string '" + sdata + "' as number.", &j);
        }
        catch(std::out_of_range& e) {
          throw json::type_error::create(0, "Number '" + sdata + "' out of range.", &j);
        }
      }
      else {
        hv.v = j;
      }
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void to_json(json& j, const HexValue& hv) { to_json(j, hv.v); }
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /** Representation of an entry in the "addressSpace" section, can be either a register or a module (or both) */

  struct JsonAddressSpaceEntry {
    std::string name;
    std::string engineeringUnit;
    std::string description;
    Access access{Access::accessNotSet};
    std::vector<size_t> triggeredByInterrupt;
    size_t numberOfElements{1};
    size_t bytesPerElement{0};

    struct Address {
      AddressType type{AddressType::IO};
      size_t channel{0};
      HexValue offset{std::numeric_limits<size_t>::max()};

      void fill(NumericAddressedRegisterInfo& info) const {
        assert(type != AddressType::addressTypeNotSet);
        info.address = offset.v;
        info.bar = channel + (type == AddressType::DMA ? 13 : 0);
      }

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Address, type, channel, offset)
    } address{AddressType::addressTypeNotSet};

    struct Representation {
      RepresentationType type{RepresentationType::FIXED_POINT};
      uint32_t width{type != RepresentationType::representationNotSet ? 32U : 0U};
      int32_t fractionalBits{0};
      bool isSigned{false};

      void fill(NumericAddressedRegisterInfo& info, size_t offset) const {
        if(type != RepresentationType::representationNotSet) {
          info.channels.emplace_back(8 * offset, NumericAddressedRegisterInfo::Type(type), width, fractionalBits,
              type != RepresentationType::IEEE754 ? isSigned : true);
        }
        else {
          Representation().fill(info, offset);
        }
      }

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Representation, type, width, fractionalBits, isSigned)
    } representation{RepresentationType::representationNotSet};

    struct ChannelTab {
      size_t numberOfElements{0};
      size_t pitch{0};

      struct Channel {
        std::string name;
        std::string engineeringUnit;
        std::string description;
        size_t offset;
        size_t bytesPerElement{4};
        Representation representation{};

        void fill(NumericAddressedRegisterInfo& info) const { representation.fill(info, offset); }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Channel, name, engineeringUnit, description, offset, bytesPerElement, representation)
      };

      std::vector<Channel> channels;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ChannelTab, numberOfElements, pitch, channels)
    };
    std::vector<ChannelTab> channelTabs;

    void fill(NumericAddressedRegisterInfo& info, const RegisterPath& parentName) const {
      info.pathName = parentName / name;

      if(triggeredByInterrupt.empty()) {
        info.registerAccess = access != Access::accessNotSet ? NumericAddressedRegisterInfo::Access(access) :
                                                               NumericAddressedRegisterInfo::Access::READ_WRITE;
      }
      else {
        if(access != Access::accessNotSet) {
          throw ChimeraTK::logic_error(
              "Register " + info.pathName + ": 'access' and 'triggeredByInterrupt' are mutually exclusive.");
        }
        info.interruptId = triggeredByInterrupt;
        info.registerAccess = NumericAddressedRegisterInfo::Access::INTERRUPT;
      }

      if(address.type != AddressType::addressTypeNotSet) {
        if(representation.type == RepresentationType::VOID) {
          throw ChimeraTK::logic_error("Address is set for void-typed register " + info.pathName);
        }
        address.fill(info);
        if(channelTabs.empty()) {
          info.elementPitchBits = bytesPerElement * 8;
          info.nElements = numberOfElements;
          representation.fill(info, 0);
        }
        else {
          if(channelTabs[0].channels.empty()) {
            throw ChimeraTK::logic_error("Empty channel definition in register " + info.pathName);
          }
          info.elementPitchBits = channelTabs[0].pitch * 8;
          info.nElements = channelTabs[0].numberOfElements;
          for(const auto& channel : channelTabs[0].channels) {
            channel.fill(info);
          }
        }
      }
      else {
        if(representation.type != RepresentationType::VOID) {
          throw ChimeraTK::logic_error("Address not set but representation given in register " + parentName / name);
        }
        if(triggeredByInterrupt.empty()) {
          throw ChimeraTK::logic_error(
              "Void-typed register " + parentName / name + " needs 'triggeredByInterrupt' entry.");
        }
        info.nElements = 0;
        info.dataDescriptor = DataDescriptor{DataDescriptor::FundamentalType::nodata};
        info.interruptId = triggeredByInterrupt;
        info.channels.emplace_back(0, NumericAddressedRegisterInfo::Type::VOID, 0, 0, false);
      }
    }

    std::vector<JsonAddressSpaceEntry> children;

    void addInfos(NumericAddressedRegisterCatalogue& catalogue, const RegisterPath& parentName) const {
      if(name.empty()) {
        throw ChimeraTK::logic_error("Entry in module " + parentName + " has no name.");
      }
      if(address.type != AddressType::addressTypeNotSet ||
          representation.type != RepresentationType::representationNotSet) {
        NumericAddressedRegisterInfo my;
        my.channels.clear(); // default constructor already creates a channel with default settings...
        fill(my, parentName);
        my.computeDataDescriptor();
        catalogue.addRegister(my);
      }
      for(const auto& child : children) {
        child.addInfos(catalogue, parentName / name);
      }
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(JsonAddressSpaceEntry, name, engineeringUnit, description, access,
        triggeredByInterrupt, numberOfElements, bytesPerElement, address, representation, children, channelTabs)
  };

  /********************************************************************************************************************/

  struct InterruptHandlerEntry {
    struct Controller {
      std::string path;
      std::set<std::string> options;
      int version{1};
      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Controller, path, options, version)
    } INTC;

    std::map<std::string, InterruptHandlerEntry> subhandler;

    void fill(const std::vector<size_t>& intId, MetadataCatalogue& metadata) const {
      if(!intId.empty()) {
        json jsonIntId;
        to_json(jsonIntId, intId);
        json jsonController;
        to_json(jsonController, INTC);
        metadata.addMetadata("!" + jsonIntId.dump(), R"({"INTC":)" + jsonController.dump() + "}");
      }

      for(const auto& [subIntId, handler] : subhandler) {
        std::vector<size_t> qualfiedSubIntId = intId;
        qualfiedSubIntId.push_back(std::stoll(subIntId));
        handler.fill(qualfiedSubIntId, metadata);
      }
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(InterruptHandlerEntry, INTC, subhandler)
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> JsonMapFileParser::Imp::parse(std::ifstream& stream) {
    // read and parse JSON data
    try {
      auto data = json::parse(stream);

      std::vector<JsonAddressSpaceEntry> addressSpace = data.at("addressSpace");
      for(const auto& entry : addressSpace) {
        entry.addInfos(catalogue, "/");
      }

      for(const auto& entry : data.at("metadata").items()) {
        if(entry.key().empty()) {
          throw ChimeraTK::logic_error(
              "Error parsing JSON map file '" + fileName + "': Metadata key must not be empty.");
        }
        if(entry.key()[0] == '_') {
          continue;
        }
        metadata.addMetadata(entry.key(), entry.value());
      }

      // backwards compatibility: interrupt handler description is expected to be in metadata
      InterruptHandlerEntry interruptHandler;
      interruptHandler.subhandler = data.at("interruptHandler");
      interruptHandler.fill({}, metadata);

      return {std::move(catalogue), std::move(metadata)};
    }
    catch(const ChimeraTK::logic_error& e) {
      throw ChimeraTK::logic_error("Error parsing JSON map file '" + fileName + "': " + e.what());
    }
    catch(const json::exception& e) {
      throw ChimeraTK::logic_error("Error parsing JSON map file '" + fileName + "': " + e.what());
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
