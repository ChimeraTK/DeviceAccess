/*
 * AddressBasedBackend.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackend.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddress.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  NumericAddressedBackend::NumericAddressedBackend(std::string mapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_STANDALONE(getRegisterAccessor_impl, 4);
    if(mapFileName != "") {
      MapFileParser parser;
      _registerMap = parser.parse(mapFileName);
      _catalogue = _registerMap->getRegisterCatalogue();
    }
    else {
      _registerMap = boost::shared_ptr<RegisterInfoMap>();
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterInfoMap::RegisterInfo> NumericAddressedBackend::getRegisterInfo(
      const RegisterPath& registerPathName) {
    if(!registerPathName.startsWith(numeric_address::BAR)) {
      boost::shared_ptr<RegisterInfo> info = _catalogue.getRegister(registerPathName);
      return boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
    }
    else {
      auto components = registerPathName.getComponents();
      if(components.size() != 3) {
        throw ChimeraTK::logic_error("Illegal numeric address: '" + (registerPathName) + "'");
      }
      auto bar = std::stoi(components[1]);
      size_t pos = components[2].find_first_of("*");
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
        throw ChimeraTK::logic_error("Illegal numeric address: '" + (registerPathName) + "'");
      }
      return boost::make_shared<RegisterInfoMap::RegisterInfo>(registerPathName, nElements, address, nBytes, bar);
    }
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::read(
      const std::string& regModule, const std::string& regName, int32_t* data, size_t dataSize, uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    read(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::write(const std::string& regModule, const std::string& regName, int32_t const* data,
      size_t dataSize, uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    write(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> NumericAddressedBackend::getRegisterMap() const { return _registerMap; }

  /********************************************************************************************************************/

  std::list<ChimeraTK::RegisterInfoMap::RegisterInfo> NumericAddressedBackend::getRegistersInModule(
      const std::string& moduleName) const {
    return _registerMap->getRegistersInModule(moduleName);
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::checkRegister(const std::string& regName, const std::string& regModule, size_t dataSize,
      uint32_t addRegOffset, uint32_t& retDataSize, uint32_t& retRegOff, uint8_t& retRegBar) const {
    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, regModule);
    if(addRegOffset % 4) {
      throw ChimeraTK::logic_error("Register offset must be divisible by 4");
    }
    if(dataSize) {
      if(dataSize % 4) {
        throw ChimeraTK::logic_error("Data size must be divisible by 4");
      }
      if(dataSize > registerInfo.nBytes - addRegOffset) {
        throw ChimeraTK::logic_error("Data size exceed register size");
      }
      retDataSize = dataSize;
    }
    else {
      retDataSize = registerInfo.nBytes;
    }
    retRegBar = registerInfo.bar;
    retRegOff = registerInfo.address + addRegOffset;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> NumericAddressedBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    boost::shared_ptr<NDRegisterAccessor<UserType>> accessor;
    // obtain register info
    boost::shared_ptr<RegisterInfo> info = getRegisterInfo(registerPathName);
    auto registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);

    // 1D or scalar register
    if(info->getNumberOfDimensions() <= 1) {
      if(registerInfo->dataType == RegisterInfoMap::RegisterInfo::Type::FIXED_POINT) {
        if (flags.has(AccessMode::raw)){
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
            new NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, true>(
                shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }else{
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
            new NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, false>(
                shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
      }
      else if(registerInfo->dataType == RegisterInfoMap::RegisterInfo::Type::IEEE754) {
        if (flags.has(AccessMode::raw)){
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
            new NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, true>(
                shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }else{
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
            new NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, false>(
                shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
      }
      else {
        throw ChimeraTK::logic_error("NumericAddressedBackend:: trying to get "
                                     "accessor for unsupported data type");
      }
    }
    // 2D multiplexed register
    else {
      accessor =
          boost::shared_ptr<NDRegisterAccessor<UserType>>(new NumericAddressedBackendMuxedRegisterAccessor<UserType>(
              registerPathName, numberOfWords, wordOffsetInRegister, shared_from_this()));
    }
    return accessor;
  }

} // namespace ChimeraTK
