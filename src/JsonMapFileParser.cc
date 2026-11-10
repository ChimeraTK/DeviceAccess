// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JsonMapFileParser.h"

#include "async/MuxedInterruptDistributor.h"
#include "JsonExtensions.h" // IWYU pragma: keep - used indirectly by nlohmann json
#include "SupportedUserTypes.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <functional>
#include <map>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  /**
   * Extract the controller type and its description (the dumped value) of an interrupt handler entry's controller
   * object. The entry reader (InterruptHandlerEntry::from_json) erases 'subhandler' and drops underscore-prefixed keys,
   * so exactly the one controller key remains; the check records that invariant rather than asserting it, so a
   * malformed descriptor throws instead of aborting an assertion-enabled build and silently picking the first key in a
   * release build. File-local: only the map file parser interprets the controller descriptors, so the json handling
   * stays out of JsonExtensions.h.
   */
  static std::pair<std::string, std::string> getInterruptControllerNameAndDescription(json const& descriptor) {
    if(!descriptor.is_object() || descriptor.size() != 1) {
      throw ChimeraTK::logic_error(
          "An interrupt controller descriptor must be an object with exactly one controller key.");
    }
    auto controllerType = descriptor.begin().key();
    auto controllerDescription = descriptor.front().dump();
    return {controllerType, controllerDescription};
  }

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
    std::optional<std::variant<std::vector<size_t>, std::string>> triggeredByInterrupt;
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
      Representation representation;
      std::optional<SelectedBy> selectedBy;
      std::map<std::string, ChannelChild> children;

      void fill(NumericAddressedRegisterInfo& info, const std::optional<SelectedBy>& inheritedSelectedBy) const {
        representation.fill(info, offset, bytesPerElement);
        if(selectedBy || inheritedSelectedBy) {
          const auto& selected = selectedBy ? *selectedBy : *inheritedSelectedBy;
          RegisterPath selReg(selected.regPath);
          selReg.setAltSeparator(".");
          info.channels.back().selectedBy.emplace(selReg, selected.value);
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

    // Record an interrupt register name reference of this entry (the string form of 'triggeredByInterrupt') under the
    // given register path, so Imp::parse can resolve it after the parse pass. Empty when the field is numeric or
    // absent. The reference string is normalised with the dot separator, as the 'selectedBy' handling does, so a
    // dot-separated target resolves.
    void recordInterruptReference(
        std::map<RegisterPath, RegisterPath>& references, const RegisterPath& registerPath) const {
      if(!triggeredByInterrupt.has_value()) {
        return;
      }
      const auto* target = std::get_if<std::string>(&*triggeredByInterrupt);
      if(target == nullptr) {
        return;
      }
      RegisterPath targetPath(*target);
      targetPath.setAltSeparator(".");
      references[registerPath] = targetPath;
    }

    // Register-level selectedBy for scalar/1D (non-2D, non-channels) registers: makes the whole register conditional
    // on a selector. For 2D registers the per-channel `Channel::selectedBy` is used instead; having both set is a map
    // authoring error.
    std::optional<SelectedBy> selectedBy;

    // Whether 'triggeredByInterrupt' declares an interrupt. A present array with no element counts as absent: an empty
    // array does not create an id-less interrupt register (the schema forbids it, but the parser does not validate
    // against the schema), so the previous empty state is preserved.
    [[nodiscard]] bool hasTriggeredByInterrupt() const {
      if(!triggeredByInterrupt.has_value()) {
        return false;
      }
      if(const auto* id = std::get_if<std::vector<size_t>>(&*triggeredByInterrupt)) {
        return !id->empty();
      }
      return true; // the string form always names a register
    }

    // The interrupt id to assign when 'triggeredByInterrupt' carries a non-empty numeric id array. The string form
    // names another register, so it has no id here yet; the id is resolved after the parse pass (see Imp::parse) and
    // the reference is collected in the reference map.
    [[nodiscard]] std::vector<size_t> interruptIdIfNumeric() const {
      if(hasTriggeredByInterrupt()) {
        if(const auto* id = std::get_if<std::vector<size_t>>(&*triggeredByInterrupt)) {
          return *id;
        }
      }
      return {};
    }

    void fill(NumericAddressedRegisterInfo& info, const std::string& name, const RegisterPath& parentName,
        bool addressSetByParent, const std::optional<SelectedBy>& inheritedSelectedBy,
        const std::string& selectedBySource) const {
      info.pathName = parentName / name;
      info.pathName.setAltSeparator(".");

      if(!hasTriggeredByInterrupt()) {
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
        // The numeric id array is assigned here; a register-name reference leaves the id empty and is filled in when
        // the reference is resolved after the parse pass.
        info.interruptId = interruptIdIfNumeric();
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
            applyRegisterSelectedBy(info, inheritedSelectedBy);
          }
          else {
            info.elementPitchBits = pitch * 8;
            info.nElements = numberOfElements;
            // Iterate the channels sorted by byte offset (see channelsInOffsetOrder) so the per-channel information
            // (and hence the channel index of the 2D accessor) stays in the natural memory order.
            for(const auto& [channelName, channel] : channelsInOffsetOrder()) {
              (void)channelName; // the channel name is the map key; the channel data carries its own offset
              channel->fill(info, inheritedSelectedBy);
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
            applyRegisterSelectedBy(info, inheritedSelectedBy);
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
        if(!hasTriggeredByInterrupt()) {
          throw ChimeraTK::logic_error(
              "Void-typed register " + parentName / name + " needs 'triggeredByInterrupt' entry.");
        }
        info.nElements = 0;
        info.dataDescriptor = DataDescriptor{DataDescriptor::FundamentalType::nodata};
        info.interruptId = interruptIdIfNumeric();
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

      // 'selectedBy' describes when data is valid to read, so it may only be applied to read-only registers.
      // A writable register (READ_WRITE or WRITE_ONLY) carrying a 'selectedBy' (own, per-channel or inherited)
      // is rejected here; the error points to the entry declaring the 'selectedBy' (the parent for inherited ones).
      if((info.registerAccess != NumericAddressedRegisterInfo::Access::READ_ONLY) &&
          (info.registerAccess != NumericAddressedRegisterInfo::Access::INTERRUPT)) {
        for(const auto& channel : info.channels) {
          if(channel.selectedBy) {
            throw ChimeraTK::logic_error("Register " + info.pathName +
                ": 'selectedBy' may only be used on read-only registers (Access::READ_ONLY or Access::INTERRUPT). "
                "The offending 'selectedBy' is declared at '" +
                selectedBySource + "'.");
          }
        }
      }
    }

    // Apply a register-level 'selectedBy' (scalar/1D registers) to the register's single channel. Must only be called
    // after 'representation.fill' created exactly one channel. A local 'selectedBy' overrides an inherited one.
    void applyRegisterSelectedBy(
        NumericAddressedRegisterInfo& info, const std::optional<SelectedBy>& inheritedSelectedBy) const {
      if(selectedBy || inheritedSelectedBy) {
        const auto& selected = selectedBy ? *selectedBy : *inheritedSelectedBy;
        RegisterPath selReg(selected.regPath);
        selReg.setAltSeparator(".");
        info.channels.back().selectedBy.emplace(selReg, selected.value);
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

    void addInfos(NumericAddressedRegisterCatalogue& catalogue,
        std::map<RegisterPath, RegisterPath>& interruptReferences, const std::string& name,
        const RegisterPath& parentName, bool addressSetByParent,
        const std::optional<SelectedBy>& inheritedSelectedBy = std::nullopt,
        const std::string& inheritedSelectedBySource = "") const {
      if(name.empty()) {
        throw ChimeraTK::logic_error("Entry in module " + parentName + " has no name.");
      }
      // The effective selector for this subtree: a local 'selectedBy' overrides an inherited one.
      const auto& effectiveSelectedBy = selectedBy ? selectedBy : inheritedSelectedBy;
      // The entry declaring the effective selector: a local 'selectedBy' is declared at this entry's path, otherwise
      // it is inherited from the entry that declared it further up (used for error messages pointing to the parent).
      RegisterPath thisPath = parentName / name;
      thisPath.setAltSeparator(".");
      const std::string effectiveSelectedBySource = selectedBy ? std::string(thisPath) : inheritedSelectedBySource;
      if(address.type != AddressType::addressTypeNotSet) {
        // New address entry. Don't use parent information
        NumericAddressedRegisterInfo my;
        my.channels.clear(); // default constructor already creates a channel with default settings...
        fill(my, name, parentName, addressSetByParent, effectiveSelectedBy, effectiveSelectedBySource);
        my.computeDataDescriptor();
        catalogue.addRegister(my);
        // A string-form entry refers by name; record the entry and, below, every derived interrupt register that
        // inherits its id, so the reference resolution covers all of them.
        recordInterruptReference(interruptReferences, my.pathName);
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
            if(channel->selectedBy || effectiveSelectedBy) {
              const auto& sb = channel->selectedBy ? *channel->selectedBy : *effectiveSelectedBy;
              auto selReg = RegisterPath(sb.regPath);
              selReg.setAltSeparator(".");
              channelSelectedBy.emplace(selReg, sb.value);
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
            recordInterruptReference(interruptReferences, slicePath);
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
              recordInterruptReference(interruptReferences, childPath);
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
        my.channels.clear(); // will be refilled from representation
        fill(my, name, parentName, addressSetByParent, effectiveSelectedBy,
            effectiveSelectedBySource); // only updates the name and the representation
        my.computeDataDescriptor();
        catalogue.addRegister(my);
        // Record the entry's own register-name reference under its path. Every address-less entry takes this branch,
        // so this is what covers a void referring register, which never has an address. A no-op for a numeric id, an
        // absent field or a non-interrupt entry.
        recordInterruptReference(interruptReferences, my.pathName);
        // Only an entry without a reference of its own that inherits its parent's interrupt id takes over the parent's
        // unresolved reference, so the child's id resolves with its parent's. A string-form entry has no id yet, so an
        // empty id together with no own reference means the id is inherited from a string-form parent. A numeric
        // parent already filled the id, and an own reference was just recorded above.
        if(my.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT && my.interruptId.empty() &&
            interruptReferences.find(my.pathName) == interruptReferences.end()) {
          const auto parentReference = interruptReferences.find(parentName);
          if(parentReference != interruptReferences.end()) {
            interruptReferences[my.pathName] = parentReference->second;
          }
        }
      }

      for(const auto& [childName, child] : children) {
        child.addInfos(catalogue, interruptReferences, childName, parentName / name,
            addressSetByParent || (address.type != AddressType::addressTypeNotSet), effectiveSelectedBy,
            effectiveSelectedBySource);
      }
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(JsonAddressSpaceEntry, engineeringUnit, description, access,
        triggeredByInterrupt, numberOfElements, bytesPerElement, pitch, address, representation, children, channels,
        doubleBuffering, selectedBy)
  };

  /********************************************************************************************************************/

  struct InterruptHandlerEntry {
    // The whole interrupt handler entry except its 'subhandler' member, kept verbatim as backend-specific metadata.
    json raw;

    std::map<std::string, InterruptHandlerEntry> subhandler;

    /**
     * The single traversal of the entry tree: when intId is non-empty it visits the entry with its qualified id and
     * its raw object, then recurses into subhandler, appending std::stoll(subIntId) to the id. Holds the subhandler
     * layout and the qualified-id construction in one place, shared by fill and augmentCatalogue.
     */
    void forEachEntry(std::vector<size_t> const& intId,
        std::function<void(std::vector<size_t> const&, nlohmann::json const&)> const& visitor) const;

    void fill(const std::vector<size_t>& intId, MetadataCatalogue& metadata) const;

    /**
     * Dispatch every entry that carries a controller object to the catalogue augmenter of its controller type. The
     * traversal runs the visitor and the recursion independently, so a nesting-only entry without a controller object
     * still reaches its subhandlers.
     */
    void augmentCatalogue(NumericAddressedRegisterCatalogue& catalogue) const;

    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void from_json(const json& j, InterruptHandlerEntry& e) {
      // An entry must be a JSON object. Its top level carries at most one controller key: the entry reader drops
      // 'subhandler' and the underscore-prefixed keys, so an entry that is not an object, or one whose top level
      // carries several keys, is a map authoring error. A nesting-only entry keeps an empty object and stays valid.
      if(!j.is_object()) {
        throw ChimeraTK::logic_error("An interrupt handler entry must be a JSON object.");
      }
      e.raw = j;
      e.raw.erase("subhandler");
      // Apply the common normalisation to the entry and to each controller data object it carries: drop keys starting
      // with an underscore and default a missing 'version' to 1. The rest of the backend-specific data structure is
      // preserved verbatim and passed on as metadata.
      // First drop underscore-prefixed keys at the entry's top level (e.g. a "_comment" sibling of the controller key).
      for(auto it = e.raw.begin(); it != e.raw.end();) {
        if(!it.key().empty() && it.key().front() == '_') {
          it = e.raw.erase(it);
        }
        else {
          ++it;
        }
      }
      if(e.raw.size() > 1) {
        throw ChimeraTK::logic_error(
            "An interrupt handler entry must carry exactly one controller key at its top level.");
      }
      // Then normalise each controller data object carried by the entry.
      for(auto& value : e.raw) {
        if(!value.is_object()) {
          continue;
        }
        if(!value.contains("version")) {
          value["version"] = 1;
        }
        for(auto it = value.begin(); it != value.end();) {
          if(!it.key().empty() && it.key().front() == '_') {
            it = value.erase(it);
          }
          else {
            ++it;
          }
        }
      }
      if(j.contains("subhandler")) {
        e.subhandler = j.at("subhandler").get<std::map<std::string, InterruptHandlerEntry>>();
      }
    }
  };

  /********************************************************************************************************************/

  void InterruptHandlerEntry::forEachEntry(std::vector<size_t> const& intId,
      std::function<void(std::vector<size_t> const&, nlohmann::json const&)> const& visitor) const {
    if(!intId.empty()) {
      visitor(intId, raw);
    }
    for(const auto& [subIntId, handler] : subhandler) {
      std::vector<size_t> qualifiedSubIntId = intId;
      qualifiedSubIntId.push_back(std::stoll(subIntId));
      handler.forEachEntry(qualifiedSubIntId, visitor);
    }
  }

  /********************************************************************************************************************/

  void InterruptHandlerEntry::fill(const std::vector<size_t>& intId, MetadataCatalogue& metadata) const {
    forEachEntry(intId, [&metadata](std::vector<size_t> const& id, nlohmann::json const& rawEntry) {
      json jsonIntId;
      jsonIntId = id;
      metadata.addMetadata("!" + jsonIntId.dump(), rawEntry.dump());
    });
  }

  /********************************************************************************************************************/

  void InterruptHandlerEntry::augmentCatalogue(NumericAddressedRegisterCatalogue& catalogue) const {
    forEachEntry({}, [&catalogue](std::vector<size_t> const& id, nlohmann::json const& rawEntry) {
      if(rawEntry.empty()) {
        return;
      }
      auto [controllerType, controllerDescription] = getInterruptControllerNameAndDescription(rawEntry);
      async::MuxedInterruptDistributorFactory::getInstance().augmentCatalogue(
          id, controllerType, controllerDescription, catalogue);
    });
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /**
   * Follow a 'triggeredByInterrupt' register-name reference chain to the final register and return its resolved
   * interrupt id. A reference names another register; that register may itself refer to yet another one, so the chain
   * is followed transitively. A missing target, a non-interrupt target and a cycle (including a register naming
   * itself) are reported as an error. File-local: only the map file parser resolves these references.
   */
  static std::vector<size_t> resolveInterruptReference(const RegisterPath& targetPath,
      const std::map<RegisterPath, RegisterPath>& references, const NumericAddressedRegisterCatalogue& catalogue) {
    std::vector<RegisterPath> chain;
    RegisterPath current = targetPath;
    while(true) {
      if(std::ranges::find(chain, current) != chain.end()) {
        std::string message = "Error parsing JSON map file: 'triggeredByInterrupt' reference cycle:";
        for(const auto& path : chain) {
          message += " " + std::string(path) + " ->";
        }
        message += " " + std::string(current);
        throw ChimeraTK::logic_error(message);
      }
      chain.push_back(current);

      // A referred register always exists in the catalogue; the reference map is keyed by the register paths the
      // parser created, so this only guards against an inconsistent map.
      if(!catalogue.hasRegister(current)) {
        throw ChimeraTK::logic_error(
            "Error parsing JSON map file: 'triggeredByInterrupt' references non-existent register '" +
            std::string(current) + "'.");
      }

      // A reference that names another reference continues the chain.
      auto it = references.find(current);
      if(it != references.end()) {
        current = it->second;
        continue;
      }

      // The end of the chain must be an interrupt register with a numeric id.
      auto target = catalogue.getBackendRegister(current);
      if(target.registerAccess != NumericAddressedRegisterInfo::Access::INTERRUPT || target.interruptId.empty()) {
        throw ChimeraTK::logic_error("Error parsing JSON map file: 'triggeredByInterrupt' target '" +
            std::string(current) + "' is not an interrupt register.");
      }
      return target.interruptId;
    }
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> JsonMapFileParser::Imp::parse(std::ifstream& stream) {
    // read and parse JSON data
    try {
      auto data = json::parse(stream);

      // Supported map format version (MAJOR.MINOR, no patch component).
      constexpr uint32_t supportedMapFormatMajor = 1;
      constexpr uint32_t supportedMapFormatMinor = 0;

      // Check the map format version: an exactly two-component MAJOR.MINOR string whose components are non-empty
      // sequences of decimal digits, compared numerically against the supported version. Each component is parsed with
      // std::from_chars into a uint32_t; a malformed or oversized component is reported through the returned error
      // code or a trailing character rather than by throwing, so no exception escapes the version check.
      std::string ver = data.at("mapFormatVersion").get<std::string>();
      uint32_t major = 0;
      uint32_t minor = 0;

      auto [p, e1] = std::from_chars(ver.data(), ver.data() + ver.size(), major);
      if(e1 != std::errc{} || p == ver.data() + ver.size() || *p++ != '.') {
        throw ChimeraTK::logic_error("Error parsing JMAP file, mapFormatVersion contains wrong version format");
      }

      auto [q, e2] = std::from_chars(p, ver.data() + ver.size(), minor);
      if(e2 != std::errc{} || q != ver.data() + ver.size()) {
        throw ChimeraTK::logic_error("Error parsing JMAP file, mapFormatVersion contains wrong version format");
      }

      if(major != supportedMapFormatMajor || minor != supportedMapFormatMinor) {
        throw ChimeraTK::logic_error("Unsupported map format version '" + ver + "'. Supported version: 1.0");
      }

      // Parse the "addressSpace" entries recursively
      std::map<RegisterPath, RegisterPath> interruptReferences;
      std::map<std::string, JsonAddressSpaceEntry> addressSpace = data.at("addressSpace");
      for(const auto& [addressSpaceName, entry] : addressSpace) {
        entry.addInfos(catalogue, interruptReferences, addressSpaceName, "/", /*addressSetByParent=*/false);
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

      // Augment the catalogue from the interrupt handler description: every 'INTC' entry, used or not, makes the
      // qualifying children of its <path>/ISR interrupt registers now, so the ids exist while the map file is parsed.
      // Otherwise the derived registers would be missing from getListOfInterrupts() and their canonical interrupt
      // paths would not resolve. It runs after the bit-range scan so isBitRange is final, and before the catalogue
      // and metadata are returned.
      InterruptHandlerEntry interruptHandlerForAugmentation;
      interruptHandlerForAugmentation.subhandler = data.at("interruptHandler");
      interruptHandlerForAugmentation.augmentCatalogue(catalogue);

      // Resolve every 'triggeredByInterrupt' register-name reference now: the implicit ISR ids created by the
      // augmentation above exist, so a derived register can be a target. Every reference is resolved before the
      // 'selectedBy' validation and the metadata handling walk the catalogue, so the interrupt ids are final.
      for(const auto& [referringPath, targetPath] : interruptReferences) {
        auto resolvedId = resolveInterruptReference(targetPath, interruptReferences, catalogue);
        auto referringRegister = catalogue.getBackendRegister(referringPath);
        referringRegister.interruptId = std::move(resolvedId);
        catalogue.modifyRegister(referringRegister);
      }

      // Validate every 'selectedBy' selector register referenced by any channel: (a) the referenced selector
      // register must exist in the catalogue, and (b) it must not itself be conditionally enabled by a 'selectedBy'
      // (a selector must be unconditionally readable, otherwise gating would depend on another gate).
      for(const auto& reg : catalogue) {
        for(const auto& channel : reg.channels) {
          if(!channel.selectedBy) {
            continue;
          }
          std::string regPath = channel.selectedBy->regPath;
          std::string channelName = reg.getRegisterName();
          // (a) The selector register must exist in the catalogue.
          if(!catalogue.hasRegister(channel.selectedBy->regPath)) {
            throw ChimeraTK::logic_error("Error parsing JSON map file '" + fileName + "': channel '" + channelName +
                "' is gated by 'selectedBy' referencing undefined register '" + regPath + "'.");
          }
          // (b) The selector register must not itself be conditionally enabled by a 'selectedBy' of its own,
          // since the gate it drives must be readable unconditionally to evaluate the selection.
          auto selectorReg = catalogue.getBackendRegister(channel.selectedBy->regPath);
          for(const auto& selectorChannel : selectorReg.channels) {
            if(selectorChannel.selectedBy) {
              throw ChimeraTK::logic_error("Error parsing JSON map file '" + fileName + "': selector register '" +
                  regPath + "' of channel '" + channelName +
                  "' is itself conditionally enabled by 'selectedBy'; a selector register must not be gated.");
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
      interruptHandlerForAugmentation.fill({}, metadata);

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
