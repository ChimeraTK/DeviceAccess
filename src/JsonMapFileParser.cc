// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JsonMapFileParser.h"

#include "JsonExtensions.h"
#include "SupportedUserTypes.h"

#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <typeinfo>

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
    friend void to_json(json& j, const HexValue& hv) { j = hv.v; }
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /** Representation of an entry in the "addressSpace" section, can be either a register or a module (or both) */

  struct JsonAddressSpaceEntry {
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
        // The two buffers of a double-buffered register must live on the same BAR.
        size_t secondaryBar =
            secondaryBufferAddress.channel + (secondaryBufferAddress.type == AddressType::DMA ? 13 : 0);
        if(secondaryBar != info.bar) {
          throw ChimeraTK::logic_error("Register " + info.pathName +
              ": double-buffered registers whose two buffers lie on different BARs are not supported (primary "
              "BAR " +
              std::to_string(info.bar) + ", secondary BAR " + std::to_string(secondaryBar) + ").");
        }
        info.doubleBuffer->enableRegisterPath = enableRegister;
        info.doubleBuffer->inactiveBufferRegisterPath = readBufferRegister;
        info.doubleBuffer->index = index;
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
          info.channels.back().bitOffset += bitShift;
          if(bitShift != 0) {
            info.isBitRange = true;
          }
        }
        else {
          Representation().fill(info, offset, bytesPerElem);
        }
      }

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Representation, type, width, fractionalBits, isSigned, bitShift)
    } representation{RepresentationType::representationNotSet};

    // 'register' is a C++ keyword, so the JSON member is deserialized manually into 'regPath'.
    struct SelectedBy {
      std::string regPath; ///< path of the register selecting this channel
      int64_t value{0};    ///< active when the 'register' equals this value

      // NOLINTNEXTLINE(readability-identifier-naming)
      friend void from_json(const nlohmann::json& j, SelectedBy& s) {
        // Both 'register' and 'value' are required for a 'selectedBy' entry. A muxed channel whose selector is
        // underspecified is a map authoring error, so reject it with a ChimeraTK::logic_error instead of silently
        // defaulting the missing field (which would otherwise make a *conditional* channel look unconditional).
        if(!j.contains("register")) {
          throw ChimeraTK::logic_error("'selectedBy' requires a 'register' member.");
        }
        if(!j.contains("value")) {
          throw ChimeraTK::logic_error("'selectedBy' requires a 'value' member.");
        }
        j.at("register").get_to(s.regPath);
        j.at("value").get_to(s.value);
      }

      // NOLINTNEXTLINE(readability-identifier-naming)
      friend void to_json(nlohmann::json& j, const SelectedBy& s) {
        j = nlohmann::json{{"register", s.regPath}, {"value", s.value}};
      }
    };

    struct ChannelChild;

    struct Channel {
      std::string engineeringUnit;
      std::string description;
      size_t offset;
      size_t bytesPerElement{4};
      Representation representation{};
      std::optional<SelectedBy> selectedBy;
      std::map<std::string, ChannelChild> children;

      void fill(NumericAddressedRegisterInfo& info) const {
        representation.fill(info, offset, bytesPerElement);
        if(selectedBy) {
          RegisterPath selReg(selectedBy.value().regPath);
          selReg.setAltSeparator(".");
          info.channels.back().selectedBy.emplace(selReg, selectedBy->value);
        }
      }

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
          Channel, engineeringUnit, description, offset, bytesPerElement, representation, selectedBy, children)
    };

    // A bit-field child of a named channel of a 2D register. Works like the top-level bit-field mechanism of a 1D
    // register, but is interpreted relative to the channel's sample word (the member byte offset/bytesPerElement).
    struct ChannelChild {
      std::string engineeringUnit;
      std::string description;
      Representation representation;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ChannelChild, engineeringUnit, description, representation)
    };

    // The channels, sorted by byte offset so that per-channel information (and thus the channel index of a
    // 2D accessor) follows the natural memory order, independent of the lexically sorted map key. Returns the
    // channel names together with the pointers so both the plain fill and the slice creation can use it.
    [[nodiscard]] std::vector<std::pair<std::string, const Channel*>> channelsInOffsetOrder() const {
      std::vector<std::pair<std::string, const Channel*>> result;
      result.reserve(channels.size());
      for(const auto& [channelName, channel] : channels) {
        result.emplace_back(channelName, &channel);
      }
      std::ranges::stable_sort(
          result, [](const auto& a, const auto& b) { return a.second->offset < b.second->offset; });
      return result;
    }

    size_t pitch{0}; ///< byte pitch between two samples of the same channel in the 2D register
    std::map<std::string, Channel> channels;

    // Register-level selectedBy for scalar/1D (non-2D, non-channels) registers: makes the whole register conditional
    // on a selector. For 2D registers the per-channel `Channel::selectedBy` is used instead; having both set is a map
    // authoring error.
    std::optional<SelectedBy> selectedBy;

    void fill(NumericAddressedRegisterInfo& info, const std::string& name, const RegisterPath& parentName,
        bool addressSetByParent) const {
      info.pathName = parentName / name;
      info.pathName.setAltSeparator(".");

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
          if(channels.empty()) {
            auto bPerElem = (bytesPerElement != 0 ? bytesPerElement : 4); // create default if not set
            info.elementPitchBits = bPerElem * 8;
            info.nElements = numberOfElements;
            representation.fill(info, 0, bPerElem);
            applyRegisterSelectedBy(info);
          }
          else {
            if(selectedBy) {
              throw ChimeraTK::logic_error("Register " + info.pathName +
                  ": 'selectedBy' must be given per channel for a 2D register, not on the register itself.");
            }
            info.elementPitchBits = pitch * 8;
            info.nElements = numberOfElements;
            // Iterate the channels sorted by byte offset (see channelsInOffsetOrder) so the per-channel information
            // (and hence the channel index of the 2D accessor) stays in the natural memory order.
            for(const auto& [channelName, channel] : channelsInOffsetOrder()) {
              (void)channelName; // the channel name is the map key; the channel data carries its own offset
              channel->fill(info);
            }
          }
        }
        else if(addressSetByParent) {
          if(channels.empty()) {
            if(representation.type == RepresentationType::representationNotSet) {
              throw ChimeraTK::logic_error("Representation not set for register " + parentName / name +
                  " which inherited the address from parent!");
            }
            // If bytesPerElement has not been set in the json file, take it from parent info
            representation.fill(info, 0, (bytesPerElement != 0 ? bytesPerElement : info.elementPitchBits / 8));
            applyRegisterSelectedBy(info);
          }
          else {
            throw ChimeraTK::logic_error("Address must be set for entries with channels: register " + info.pathName);
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

      info.description = description;
      info.engineeringUnit = engineeringUnit;
    }

    // Apply a register-level 'selectedBy' (scalar/1D registers) to the register's single channel. Must only be called
    // after 'representation.fill' created exactly one channel.
    void applyRegisterSelectedBy(NumericAddressedRegisterInfo& info) const {
      if(selectedBy) {
        RegisterPath selReg(selectedBy.value().regPath);
        selReg.setAltSeparator(".");
        info.channels.back().selectedBy.emplace(selReg, selectedBy->value);
      }
    }

    // Build the ChannelInfo of a channel or bit-field child slice from its 'representation'. The raw type always
    // spans the whole sample word (wordBits), so the slice reads the element with its full word width in the
    // underlying transport and extracts the range via the bit offset/width. Shared by the parent channel slice and
    // the bit-field child slice creation.
    static NumericAddressedRegisterInfo::ChannelInfo makeChannelInfo(const Representation& rep, size_t wordBits,
        const std::optional<NumericAddressedRegisterInfo::SelectedBy>& selectedBy) {
      return {rep.bitShift, NumericAddressedRegisterInfo::Type(rep.type), rep.width, rep.fractionalBits,
          rep.type != RepresentationType::IEEE754 ? rep.isSigned : true, DataType("int" + std::to_string(wordBits)),
          selectedBy};
    }

    // Add a slice register to the catalogue and, for a double-buffered register, its two buffer-view registers
    // BUF0/BUF1, exactly as the parent 2D register's BUF0/BUF1 block but folded to the slice. The slice's own
    // (already channel-shifted) secondary buffer address is reused for the BUF1 view, which is why both the parent
    // channel slice and the inherited child slice can share this helper.
    static void addSliceWithBufferViews(NumericAddressedRegisterCatalogue& catalogue,
        NumericAddressedRegisterInfo& slice, const RegisterPath& slicePath) {
      catalogue.addRegister(slice);
      if(!slice.doubleBuffer.has_value()) {
        return;
      }
      NumericAddressedRegisterInfo sliceBuf0 = slice;
      sliceBuf0.pathName = slicePath + "/BUF0";
      sliceBuf0.doubleBuffer.reset();
      sliceBuf0.registerAccess = NumericAddressedRegisterInfo::Access::READ_ONLY;
      sliceBuf0.computeDataDescriptor();
      catalogue.addRegister(sliceBuf0);
      NumericAddressedRegisterInfo sliceBuf1 = slice;
      sliceBuf1.pathName = slicePath + "/BUF1";
      sliceBuf1.doubleBuffer.reset();
      sliceBuf1.address = slice.doubleBuffer->address;
      sliceBuf1.registerAccess = NumericAddressedRegisterInfo::Access::READ_ONLY;
      sliceBuf1.computeDataDescriptor();
      catalogue.addRegister(sliceBuf1);
    }

    std::map<std::string, JsonAddressSpaceEntry> children;

    void addInfos(NumericAddressedRegisterCatalogue& catalogue, const std::string& name, const RegisterPath& parentName,
        bool addressSetByParent) const {
      if(name.empty()) {
        throw ChimeraTK::logic_error("Entry in module " + parentName + " has no name.");
      }
      if(address.type != AddressType::addressTypeNotSet) {
        // New address entry. Don't use parent information
        NumericAddressedRegisterInfo my;
        my.channels.clear(); // default constructor already creates a channel with default settings...
        fill(my, name, parentName, addressSetByParent);
        my.computeDataDescriptor();
        catalogue.addRegister(my);
        if(!channels.empty()) {
          // create one register entry per named channel of the 2D register: a read-only 1D slice of the
          // 2D register. The channel's byte offset is folded into the address (so bitOffset == 0), and the full
          // element pitch is kept as the stride between samples. Iterate sorted by byte offset (see
          // channelsInOffsetOrder) so the created slice registers follow the natural memory order.
          for(const auto& [channelName, channel] : channelsInOffsetOrder()) {
            RegisterPath slicePath = my.pathName / channelName;
            slicePath.setAltSeparator(".");
            // A channel slice whose path collides with an already existing register is a map authoring error.
            if(catalogue.hasRegister(slicePath)) {
              throw ChimeraTK::logic_error(
                  "Channel slice '" + (slicePath) + "' collides with an already existing register.");
            }
            const auto& rep = channel->representation;

            std::optional<NumericAddressedRegisterInfo::SelectedBy> channelSelectedBy = std::nullopt;
            if(channel->selectedBy) {
              auto selReg = RegisterPath(channel->selectedBy->regPath);
              selReg.setAltSeparator(".");
              channelSelectedBy.emplace(selReg, channel->selectedBy->value);
            }
            auto wordBits = channel->bytesPerElement * 8;
            // A channel slice of a non-interrupt 2D register is read-only: writing to a single channel of a 2D
            // register would require a read-modify-write cycle across the channels, which is deliberately not
            // supported. A slice of an interrupt-driven 2D register additionally advertises wait_for_new_data,
            // since the whole 2D register (including all its channel slices) updates with the same interrupt.
            auto sliceAccessType = (my.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT) ?
                NumericAddressedRegisterInfo::Access::INTERRUPT :
                NumericAddressedRegisterInfo::Access::READ_ONLY;
            NumericAddressedRegisterInfo slice(slicePath, my.bar, my.address + channel->offset, my.nElements,
                my.elementPitchBits, {makeChannelInfo(rep, wordBits, channelSelectedBy)}, sliceAccessType,
                my.interruptId, my.doubleBuffer);
            // The slice's double-buffer configuration inherits the parent's, but its secondary buffer address
            // is the parent's shifted by the channel byte offset, matching the slice's own data address and the
            // slice's BUF1 buffer-view register created below.
            if(slice.doubleBuffer.has_value()) {
              slice.doubleBuffer->address += channel->offset;
            }
            slice.isBitRange = (rep.bitShift != 0);
            slice.computeDataDescriptor();
            slice.engineeringUnit = channel->engineeringUnit;
            slice.description = channel->description;
            addSliceWithBufferViews(catalogue, slice, slicePath);
            // Create one read-only bit-range slice per bit-field child of the channel, at <channel>/<child>.
            // The child slice behaves exactly like the parent channel slice (same address, stride, interrupt and
            // double-buffer inheritance, including the BUF0/BUF1 buffer views), but extracts the child's bit range
            // from every sample word. The word context is the channel's byte offset and bytesPerElement.
            for(const auto& [childName, child] : channel->children) {
              const auto& crep = child.representation;
              // A child that is not a proper bit range of the channel word is unsupported and is ignored, so newer
              // map files that use a not yet supported child feature still parse for the supported parts.
              if((crep.bitShift == 0 && crep.width == wordBits) || (crep.bitShift + crep.width > wordBits)) {
                continue;
              }
              RegisterPath childPath = slicePath / childName;
              childPath.setAltSeparator(".");
              // A child slice whose path collides with an already existing register is a map authoring error.
              if(catalogue.hasRegister(childPath)) {
                throw ChimeraTK::logic_error(
                    "Child slice '" + (childPath) + "' collides with an already existing register.");
              }
              NumericAddressedRegisterInfo childSlice = slice;
              childSlice.pathName = childPath;
              childSlice.channels = {makeChannelInfo(crep, wordBits, channelSelectedBy)};
              childSlice.isBitRange = true;
              childSlice.engineeringUnit = child.engineeringUnit;
              childSlice.description = child.description;
              childSlice.computeDataDescriptor();
              addSliceWithBufferViews(catalogue, childSlice, childPath);
            }
          }
        }
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
          buf1Register.computeDataDescriptor();
          catalogue.addRegister(buf1Register);
        }
      }
      else if(representation.type != RepresentationType::representationNotSet) {
        // take over parent address (except void interrupt registers which don't have an address)
        auto my = catalogue.getBackendRegister(parentName);
        my.channels.clear();                            // will be refilled from representation
        fill(my, name, parentName, addressSetByParent); // only updates the name and the representation
        my.computeDataDescriptor();
        catalogue.addRegister(my);
      }

      for(const auto& [childName, child] : children) {
        child.addInfos(catalogue, childName, parentName / name,
            addressSetByParent || (address.type != AddressType::addressTypeNotSet));
      }
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(JsonAddressSpaceEntry, engineeringUnit, description, access,
        triggeredByInterrupt, numberOfElements, bytesPerElement, pitch, address, representation, children, channels,
        doubleBuffering, selectedBy)
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

      std::map<std::string, JsonAddressSpaceEntry> addressSpace = data.at("addressSpace");
      for(const auto& [addressSpaceName, entry] : addressSpace) {
        entry.addInfos(catalogue, addressSpaceName, "/", /*addressSetByParent=*/false);
      }

      // Scan the catalogue for bit ranges.
      // Afterwards, scan again for registers which have bit shift 0, a width smaller than their element size and that
      // share their starting address with a bit range. They have to become bit ranges as well.
      std::set<std::pair<uint64_t, uint64_t>> addressesWithBitRange;
      for(auto& reg : catalogue) {
        if(reg.isBitRange) {
          addressesWithBitRange.insert({reg.bar, reg.address});
        }
      }
      if(addressesWithBitRange.size()) {
        // Compute the element data width (bytes per element times 8) from the single channel's raw type. This equals
        // the element pitch only for non-strided registers; a strided channel slice has a larger element pitch than
        // its element data width, so such slices must not be reclassified as bit ranges even when their element data
        // width is smaller than the pitch or they share their address with a bit range.
        auto elementDataWidth = [](const NumericAddressedRegisterInfo::ChannelInfo& c) {
          // Obtain the element data width (bytes per element times 8) from the channel's raw type via
          // DataType::getNumberOfBytes(); no std::bad_cast can arise because the switch covers every DataType value.
          return c.getRawType().getNumberOfBytes() * 8;
        };
        for(auto& reg : catalogue) {
          if((reg.channels.size() == 1) && (reg.channels[0].bitOffset == 0) &&
              (reg.channels[0].width < elementDataWidth(reg.channels[0])) &&
              (reg.elementPitchBits == elementDataWidth(reg.channels[0]))) {
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
