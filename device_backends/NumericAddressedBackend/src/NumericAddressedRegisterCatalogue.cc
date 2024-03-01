// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddress.h"
#include "predicates.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ChimeraTK {

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo::NumericAddressedRegisterInfo(RegisterPath const& pathName_, uint32_t nElements_,
      uint64_t address_, uint32_t nBytes_, uint64_t bar_, uint32_t width_, int32_t nFractionalBits_, bool signedFlag_,
      Access dataAccess_, Type dataType_, std::vector<uint32_t> interruptId_)
  : pathName(pathName_), nElements(nElements_), elementPitchBits(nElements_ > 0 ? nBytes_ / nElements_ * 8 : 0),
    bar(bar_), address(address_), registerAccess(dataAccess_), interruptId(interruptId_),
    channels({{0, dataType_, width_, nFractionalBits_, signedFlag_}}) {
    assert(channels.size() == 1);

    // make sure . and / is treated as similar as possible
    pathName.setAltSeparator(".");

    // consistency checks
    if(nBytes_ > 0 && nElements_ > 0) {
      if(nBytes_ % nElements_ != 0) {
        // nBytes_ must be divisible by nElements_
        throw logic_error("Number of bytes is not a multiple of number of elements for register " + pathName +
            ". Check your map file!");
      }
    }

    computeDataDescriptor();
  }

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo::NumericAddressedRegisterInfo(RegisterPath const& pathName_, uint64_t bar_,
      uint64_t address_, uint32_t nElements_, uint32_t elementPitchBits_, std::vector<ChannelInfo> channelInfo_,
      Access dataAccess_, std::vector<uint32_t> interruptId_)
  : pathName(pathName_), nElements(nElements_), elementPitchBits(elementPitchBits_), bar(bar_), address(address_),
    registerAccess(dataAccess_), interruptId(interruptId_), channels(std::move(channelInfo_)) {
    assert(!channels.empty());

    // make sure . and / is treated as similar as possible
    pathName.setAltSeparator(".");

    computeDataDescriptor();
  }

  /********************************************************************************************************************/

  void NumericAddressedRegisterInfo::computeDataDescriptor() {
    // Determine DataDescriptor. If there are multiple channels, use the "biggest" data type.
    Type dataType = Type::VOID;
    uint32_t width = 0;
    int32_t nFractionalBits = 0;
    bool signedFlag = false;
    for(auto& c : channels) {
      if(int(c.dataType) > int(dataType)) dataType = c.dataType;
      if(c.width + c.nFractionalBits + c.signedFlag > width + nFractionalBits + signedFlag) {
        width = c.width;
        nFractionalBits = c.nFractionalBits;
        signedFlag = c.signedFlag;
      }
    }

    // set raw data type
    DataType rawDataInfo{DataType::none};
    if(channels.size() == 1) {
      if(elementPitchBits == 0) {
        rawDataInfo = DataType::Void;
      }
      else if(elementPitchBits == 8) {
        rawDataInfo = DataType::int8;
      }
      else if(elementPitchBits == 16) {
        rawDataInfo = DataType::int16;
      }
      else if(elementPitchBits == 32) {
        rawDataInfo = DataType::int32;
      }
      else if(elementPitchBits == 64) {
        rawDataInfo = DataType::int64;
      }
      else {
        if(dataType != Type::ASCII) {
          throw ChimeraTK::logic_error(
              "Unsupported raw size: " + std::to_string(elementPitchBits) + " bits in register " + pathName);
        }
      }
    }

    // set "cooked" data type
    if(dataType == Type::IEEE754) {
      if(width == 32) {
        // Largest possible number +- 3e38, smallest possible number 1e-45
        // However, the actual precision is only 23+1 bit, which is < 1e9 relevant
        // digits. Hence, we don't have to add the 3e38 and the 1e45, but just add
        // the leading 0. comma and sign to the largest 45 digits
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
            false,                                                                // isIntegral
            true,                                                                 // isSigned
            3 + 45,                                                               // nDigits
            45,                                                                   // nFractionalDigits
            rawDataInfo); // we have integer in the transport layer, or none if multiplexed
      }
      else if(width == 64) {
        // smallest possible 5e-324, largest 2e308
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
            false,                                                                // isIntegral
            true,                                                                 // isSigned
            3 + 325,                                                              // nDigits
            325,                                                                  // nFractionalDigits
            rawDataInfo);
      }
      else {
        throw logic_error("Wrong data width for data type IEEE754 for register " + pathName + ". Check your map file!");
      }
    }
    else if(dataType == Type::FIXED_POINT) {
      if(width > 1) { // numeric type

        if(nFractionalBits > 0) {
          auto nDigits = static_cast<size_t>(
              std::ceil(std::log10(std::pow(2, width))) + (signedFlag ? 1 : 0) + (nFractionalBits != 0 ? 1 : 0));
          size_t nFractionalDigits = std::ceil(std::log10(std::pow(2, nFractionalBits)));

          dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
              false,                                                                // isIntegral
              signedFlag,                                                           // isSigned
              nDigits, nFractionalDigits, rawDataInfo);
        }
        else {
          auto nDigits =
              static_cast<size_t>(std::ceil(std::log10(std::pow(2, width + nFractionalBits))) + (signedFlag ? 1 : 0));

          dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
              true,                                                                 // isIntegral
              signedFlag,                                                           // isSigned
              nDigits, 0, rawDataInfo);
        }
      }
      else if(width == 1) { // boolean
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::boolean, true, false, 1, 0, rawDataInfo);
      }
      else { // width == 0 -> nodata
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::nodata, false, false, 0, 0, rawDataInfo);
      }
    }
    else if(dataType == Type::ASCII) {
      dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::string, false, false, 0, 0, rawDataInfo);
    }
    else if(dataType == Type::VOID) {
      dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::nodata, false, false, 0, 0, rawDataInfo);
    }
  }

  /********************************************************************************************************************/

  bool NumericAddressedRegisterInfo::operator==(const ChimeraTK::NumericAddressedRegisterInfo& rhs) const {
    return (address == rhs.address) && (bar == rhs.bar) && (nElements == rhs.nElements) && (channels == rhs.channels) &&
        (pathName == rhs.pathName) && (elementPitchBits == rhs.elementPitchBits) &&
        (registerAccess == rhs.registerAccess) && (getNumberOfDimensions() == rhs.getNumberOfDimensions()) &&
        (interruptId == rhs.interruptId);
  }
  /********************************************************************************************************************/

  bool NumericAddressedRegisterInfo::operator!=(const ChimeraTK::NumericAddressedRegisterInfo& rhs) const {
    return !operator==(rhs);
  }

  /********************************************************************************************************************/

  bool NumericAddressedRegisterInfo::ChannelInfo::operator==(const ChannelInfo& rhs) const {
    return bitOffset == rhs.bitOffset && dataType == rhs.dataType && width == rhs.width &&
        nFractionalBits == rhs.nFractionalBits && signedFlag == rhs.signedFlag;
  }

  /********************************************************************************************************************/

  DataType NumericAddressedRegisterInfo::ChannelInfo::getRawType() const {
    if(width > 16) return DataType::int32;
    if(width > 8) return DataType::int16;
    return DataType::int8;
  }

  /********************************************************************************************************************/

  bool NumericAddressedRegisterInfo::ChannelInfo::operator!=(const ChannelInfo& rhs) const {
    return !operator==(rhs);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  NumericAddressedRegisterInfo NumericAddressedRegisterCatalogue::getBackendRegister(
      const RegisterPath& registerPathName) const {
    auto path = registerPathName;
    path.setAltSeparator(".");

    if(path.startsWith(numeric_address::BAR())) {
      // special treatment for numeric addresses
      auto components = path.getComponents();
      if(components.size() != 3) {
        throw ChimeraTK::logic_error("Illegal numeric address: '" + (path) + "'");
      }
      auto bar = std::stoi(components[1]);
      size_t pos = components[2].find_first_of('*');
      auto address = std::stoi(components[2].substr(0, pos));
      size_t nBytes;
      if(pos != std::string::npos) {
        nBytes = std::stoi(components[2].substr(pos + 1));
      }
      else {
        nBytes = sizeof(int32_t);
      }
      auto nElements = nBytes / sizeof(int32_t);
      if(nBytes == 0 || nBytes % sizeof(int32_t) != 0) {
        throw ChimeraTK::logic_error("Illegal numeric address: '" + (path) + "'");
      }
      return NumericAddressedRegisterInfo(path, nElements, address, nBytes, bar);
    }
    if(path.startsWith("!")) {
      auto canonicalInterrupt = _canonicalInterrupts.find(path);
      if(canonicalInterrupt == _canonicalInterrupts.end()) {
        throw ChimeraTK::logic_error("Illegal canonical interrupt path: '" + (path) + "'");
      }
      return NumericAddressedRegisterInfo(path, 0, 0, 0, 0, 0, 0, false,
          NumericAddressedRegisterInfo::Access::INTERRUPT, NumericAddressedRegisterInfo::Type::VOID,
          canonicalInterrupt->second);
    }
    return BackendRegisterCatalogue::getBackendRegister(path);
  }

  /********************************************************************************************************************/

  [[nodiscard]] bool NumericAddressedRegisterCatalogue::hasRegister(const RegisterPath& registerPathName) const {
    if(registerPathName.startsWith(numeric_address::BAR())) {
      /// TODO check whether given address is correct
      return true;
    }
    if(_canonicalInterrupts.find(registerPathName) != _canonicalInterrupts.end()) {
      return true;
    }
    return BackendRegisterCatalogue::hasRegister(registerPathName);
  }

  /********************************************************************************************************************/

  const std::set<std::vector<uint32_t>>& NumericAddressedRegisterCatalogue::getListOfInterrupts() const {
    return _listOfInterrupts;
  }

  /********************************************************************************************************************/

  void NumericAddressedRegisterCatalogue::addRegister(const NumericAddressedRegisterInfo& registerInfo) {
    if(registerInfo.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT) {
      _listOfInterrupts.insert(registerInfo.interruptId);
      RegisterPath canonicalName = "!" + std::to_string(registerInfo.interruptId.front());
      std::vector<uint32_t> canonicalID = {registerInfo.interruptId.front()};
      _canonicalInterrupts[canonicalName] = canonicalID;
      for(auto subId = ++registerInfo.interruptId.begin(); subId != registerInfo.interruptId.end(); ++subId) {
        canonicalName += ":" + std::to_string(*subId);
        canonicalID.push_back(*subId);
        _canonicalInterrupts[canonicalName] = canonicalID;
      }
    }
    BackendRegisterCatalogue<NumericAddressedRegisterInfo>::addRegister(registerInfo);
  }

  /********************************************************************************************************************/

  std::unique_ptr<BackendRegisterCatalogueBase> NumericAddressedRegisterCatalogue::clone() const {
    std::unique_ptr<BackendRegisterCatalogueBase> c = std::make_unique<NumericAddressedRegisterCatalogue>();
    auto* casted_c = dynamic_cast<NumericAddressedRegisterCatalogue*>(c.get());
    fillFromThis(casted_c);
    casted_c->_listOfInterrupts = _listOfInterrupts;
    casted_c->_canonicalInterrupts = _canonicalInterrupts;
    return c;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
