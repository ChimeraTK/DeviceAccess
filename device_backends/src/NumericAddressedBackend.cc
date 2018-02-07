/*
 * AddressBasedBackend.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackend.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "DeviceException.h"
#include "NumericAddress.h"
#include "MapFileParser.h"

namespace ChimeraTK {

  NumericAddressedBackend::NumericAddressedBackend(std::string mapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
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

  boost::shared_ptr<RegisterInfoMap::RegisterInfo> NumericAddressedBackend::getRegisterInfo(const RegisterPath &registerPathName) {
    if(!registerPathName.startsWith(numeric_address::BAR)) {
      boost::shared_ptr<RegisterInfo> info = _catalogue.getRegister(registerPathName);
      return boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
    }
    else {
      boost::shared_ptr<RegisterInfoMap::RegisterInfo> info(new RegisterInfoMap::RegisterInfo());
      auto components = registerPathName.getComponents();
      if(components.size() != 3) {
        throw DeviceException("Illegal numeric address: '"+(registerPathName)+"'", DeviceException::REGISTER_DOES_NOT_EXIST);
      }
      info->bar = std::stoi(components[1]);
      size_t pos = components[2].find_first_of("*");
      info->address = std::stoi(components[2].substr(0,pos));
      if(pos != std::string::npos) {
        info->nBytes = std::stoi(components[2].substr(pos+1));
      }
      else {
        info->nBytes = sizeof(int32_t);
      }
      info->nElements = info->nBytes/sizeof(int32_t);
      if(info->nBytes == 0 || info->nBytes % sizeof(int32_t) != 0) {
        throw DeviceException("Illegal numeric address: '"+(registerPathName)+"'", DeviceException::REGISTER_DOES_NOT_EXIST);
      }
      return info;
    }
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::read(const std::string &regModule, const std::string &regName,
      int32_t *data, size_t dataSize, uint32_t addRegOffset) {

    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    read(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::write(const std::string &regModule, const std::string &regName,
      int32_t const *data, size_t dataSize, uint32_t addRegOffset) {

    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    write(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> NumericAddressedBackend::getRegisterMap() const {
    return _registerMap;
  }

  /********************************************************************************************************************/

  std::list<ChimeraTK::RegisterInfoMap::RegisterInfo> NumericAddressedBackend::getRegistersInModule(
      const std::string &moduleName) const {
    return _registerMap->getRegistersInModule(moduleName);
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::checkRegister(const std::string &regName,
      const std::string &regModule, size_t dataSize,
      uint32_t addRegOffset, uint32_t &retDataSize,
      uint32_t &retRegOff, uint8_t &retRegBar) const {

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, regModule);
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4", DeviceException::WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4", DeviceException::WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.nBytes - addRegOffset) {
        throw DeviceException("Data size exceed register size", DeviceException::WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.nBytes;
    }
    retRegBar = registerInfo.bar;
    retRegOff = registerInfo.address + addRegOffset;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > NumericAddressedBackend::getRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    boost::shared_ptr< NDRegisterAccessor<UserType> >  accessor;
    // obtain register info
    boost::shared_ptr<RegisterInfo> info = getRegisterInfo(registerPathName);
    // 1D or scalar register
    if(info->getNumberOfDimensions() <= 1) {
      accessor = boost::shared_ptr< NDRegisterAccessor<UserType> >(
          new NumericAddressedBackendRegisterAccessor<UserType>(shared_from_this(), registerPathName,
              numberOfWords, wordOffsetInRegister, flags) );
    }
    // 2D multiplexed register
    else {
      accessor = boost::shared_ptr< NDRegisterAccessor<UserType> >(
          new NumericAddressedBackendMuxedRegisterAccessor<UserType>(registerPathName, numberOfWords,
              wordOffsetInRegister, shared_from_this()) );
    }
    // allow plugins to decorate the accessor and return it
    return decorateRegisterAccessor(registerPathName, accessor);
  }

} // namespace ChimeraTK
