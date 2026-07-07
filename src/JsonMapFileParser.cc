// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JsonMapFileParser.h"

#include "JsonExtensions.h"

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

    static std::set<std::pair<uint64_t, uint64_t>> addressesWithBitRange;
  };
  std::set<std::pair<uint64_t, uint64_t>> JsonMapFileParser::Imp::addressesWithBitRange;

  /********************************************************************************************************************/

  JsonMapFileParser::JsonMapFileParser(std::string fileName) : theImp(std::make_unique<Imp>(std::move(fileName))) {}

  JsonMapFileParser::~JsonMapFileParser() = default;

  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> JsonMapFileParser::parse(std::ifstream& stream) {
    return theImp->parse(stream);
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
    friend void to_json(json& j, const HexValue& hv) { j = hv.v; }
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

    struct DoubleBufferingInfo {
      struct SecondAddress {
        AddressType type{AddressType::DMA};
        size_t channel{0};
        HexValue offset{0};

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(SecondAddress, type, channel, offset)
      };

      SecondAddress secondaryBufferAddress;
      std::string enableRegister;
      std::string readBufferRegister;
      size_t index{0};

      void fill(NumericAddressedRegisterInfo& info) const {
        info.doubleBuffer->address = secondaryBufferAddress.offset.v;
        info.doubleBuffer->enableRegisterPath = enableRegister;
        info.doubleBuffer->inactiveBufferRegisterPath = readBufferRegister;
      }

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
          DoubleBufferingInfo, secondaryBufferAddress, enableRegister, readBufferRegister, index)
    };
    std::optional<DoubleBufferingInfo> doubleBuffering;

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

    // Basic representation without sub elements
    struct Representation {
      RepresentationType type{RepresentationType::FIXED_POINT};
      uint32_t width{type != RepresentationType::representationNotSet ? 32U : 0U};
      int32_t fractionalBits{0};
      bool isSigned{false};
      uint32_t bitShift{0};

      void fill(NumericAddressedRegisterInfo& info, size_t offset, size_t bytesPerElem) const {
        if(type != RepresentationType::representationNotSet) {
          info.channels.emplace_back(8 * offset, NumericAddressedRegisterInfo::Type(type), width, fractionalBits,
              type != RepresentationType::IEEE754 ? isSigned : true,
              DataType("int" + std::to_string(bytesPerElem * 8)));
          info.channels[0].bitOffset = bitShift;
          if(bitShift != 0) {
            info.isBitRange = true;
            JsonMapFileParser::Imp::addressesWithBitRange.insert({info.bar, info.address});
          }
        }
        else {
          Representation().fill(info, offset, bytesPerElem);
        }
      }

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Representation, type, width, fractionalBits, isSigned, bitShift)
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

        void fill(NumericAddressedRegisterInfo& info) const { representation.fill(info, offset, bytesPerElement); }

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Channel, name, engineeringUnit, description, offset, bytesPerElement, representation)
      };

      std::vector<Channel> channels;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ChannelTab, numberOfElements, pitch, channels)
    };
    std::vector<ChannelTab> channelTabs;

    void fill(NumericAddressedRegisterInfo& info, const RegisterPath& parentName, bool addressSetByParent) const {
      info.pathName = parentName / name;

      if(triggeredByInterrupt.empty()) {
        if(access != Access::accessNotSet) {
          info.registerAccess = NumericAddressedRegisterInfo::Access(access);
        }
        else if(!addressSetByParent) {
          info.registerAccess = NumericAddressedRegisterInfo::Access::READ_WRITE;
        }
      }
      else {
        if(access != Access::accessNotSet) {
          throw ChimeraTK::logic_error(
              "Register " + info.pathName + ": 'access' and 'triggeredByInterrupt' are mutually exclusive.");
        }
        info.interruptId = triggeredByInterrupt;
        info.registerAccess = NumericAddressedRegisterInfo::Access::INTERRUPT;
      }

      if(representation.type != RepresentationType::VOID) {
        if(address.type != AddressType::addressTypeNotSet) {
          address.fill(info);
          if(channelTabs.empty()) {
            info.elementPitchBits = bytesPerElement * 8;
            info.nElements = numberOfElements;
            representation.fill(info, 0, bytesPerElement);
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
        else if(addressSetByParent) {
          if(channelTabs.empty()) {
            if(representation.type == RepresentationType::representationNotSet) {
              throw ChimeraTK::logic_error("Representation not set for register " + parentName / name +
                  " which inherited the address from parent!");
            }
            representation.fill(info, 0, bytesPerElement);
          }
          else {
            throw ChimeraTK::logic_error("Address must be set for entries in channel tabs: register " + info.pathName);
          }
        }
        else {
          throw ChimeraTK::logic_error("Address not set but representation given in register " + parentName / name);
        }
      }
      else {
        // VOID registers have no address — they are pure interrupt sources
        if(address.type != AddressType::addressTypeNotSet) {
          throw ChimeraTK::logic_error("Address is set for void-typed register " + info.pathName);
        }
        if(triggeredByInterrupt.empty()) {
          throw ChimeraTK::logic_error(
              "Void-typed register " + parentName / name + " needs 'triggeredByInterrupt' entry.");
        }
        info.nElements = 0;
        info.dataDescriptor = DataDescriptor{DataDescriptor::FundamentalType::nodata};
        info.interruptId = triggeredByInterrupt;
        info.registerAccess = NumericAddressedRegisterInfo::Access::INTERRUPT;
        info.channels.clear();
        info.channels.emplace_back(0, NumericAddressedRegisterInfo::Type::VOID, 0, 0, false);
      }

      if(doubleBuffering) {
        info.doubleBuffer.emplace();
        doubleBuffering->fill(info);
      }
      else {
        info.doubleBuffer.reset();
      }
    }

    std::vector<JsonAddressSpaceEntry> children;

    void addInfos(
        NumericAddressedRegisterCatalogue& catalogue, const RegisterPath& parentName, bool addressSetByParent) const {
      if(name.empty()) {
        throw ChimeraTK::logic_error("Entry in module " + parentName + " has no name.");
      }
      if(address.type != AddressType::addressTypeNotSet) {
        // New address entry. Don't use parent information
        NumericAddressedRegisterInfo my;
        my.channels.clear(); // default constructor already creates a channel with default settings...
        fill(my, parentName, addressSetByParent);
        my.computeDataDescriptor();
        catalogue.addRegister(my);
        if(doubleBuffering.has_value()) {
          // Create the .buf0 register as a copy of the main one
          NumericAddressedRegisterInfo buf0Register = my;
          buf0Register.pathName = my.pathName + "/BUF0";
          buf0Register.doubleBuffer.reset(); // it's a simple view of the buffer
          buf0Register.registerAccess = NumericAddressedRegisterInfo::Access::READ_ONLY;
          buf0Register.computeDataDescriptor();
          catalogue.addRegister(buf0Register);
          NumericAddressedRegisterInfo buf1Register = my;
          buf1Register.pathName = my.pathName + "/BUF1";
          buf1Register.doubleBuffer.reset(); // it's a simple view of the buffer
          buf1Register.address = doubleBuffering->secondaryBufferAddress.offset.v;
          buf1Register.registerAccess = NumericAddressedRegisterInfo::Access::READ_ONLY;
          // buf1Register.bar = doubleBuffering->secondBufferAddress.channel +
          //     (doubleBuffering->secondaryBufferAddress.type == AddressType::DMA ? 13 : 0);
          buf1Register.computeDataDescriptor();
          catalogue.addRegister(buf1Register);
        }
      }
      else if(representation.type != RepresentationType::representationNotSet) {
        auto my = catalogue.getBackendRegister(parentName);
        my.channels.clear();                      // will be refilled from representation
        fill(my, parentName, addressSetByParent); // only updates the name and the representation
        my.computeDataDescriptor();
        catalogue.addRegister(my);
      }

      for(const auto& child : children) {
        child.addInfos(
            catalogue, parentName / name, addressSetByParent || (address.type != AddressType::addressTypeNotSet));
      }
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(JsonAddressSpaceEntry, name, engineeringUnit, description, access,
        triggeredByInterrupt, numberOfElements, bytesPerElement, address, representation, children, channelTabs,
        doubleBuffering)
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
        jsonIntId = intId;
        json jsonController;
        jsonController = INTC;
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
        entry.addInfos(catalogue, "/", /*addressSetByParent=*/false);
      }
      if(addressesWithBitRange.size()) {
        // Scan the catalogue for registers what have bit shift 0, a width smaller than their element size and that
        // share their starting address with a bit range. They have to become bit ranges as well.
        for(auto& reg : catalogue) {
          if((reg.channels.size() == 1) && (reg.channels[0].bitOffset == 0) &&
              (reg.channels[0].width < reg.elementPitchBits)) {
            if(addressesWithBitRange.find({reg.bar, reg.address}) != addressesWithBitRange.end()) {
              reg.isBitRange = true;
            }
          }
        }
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
