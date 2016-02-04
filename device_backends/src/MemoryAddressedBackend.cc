/*
 * AddressBasedBackend.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#include "MemoryAddressedBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  MemoryAddressedBackend::MemoryAddressedBackend(std::string mapFileName) {
    if(mapFileName != "") {
      MapFileParser parser;
      _registerMap = parser.parse(mapFileName);
    }
    else {
      _registerMap = boost::shared_ptr<RegisterInfoMap>();
    }
  }

/********************************************************************************************************************/

  void MemoryAddressedBackend::read(const std::string &regModule, const std::string &regName,
      int32_t *data, size_t dataSize, uint32_t addRegOffset) {

    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    read(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackend::write(const std::string &regModule, const std::string &regName,
      int32_t const *data, size_t dataSize, uint32_t addRegOffset) {

    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    write(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<mtca4u::RegisterAccessor> MemoryAddressedBackend::getRegisterAccessor(
      const std::string &registerName,
      const std::string &module) {

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(registerName, registerInfo, module);
    return boost::shared_ptr<RegisterAccessor>(
        new MemoryAddressedBackendRegisterAccessor(registerInfo, boost::static_pointer_cast<DeviceBackend>(shared_from_this()) ));
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> MemoryAddressedBackend::getRegisterMap() const {
    return _registerMap;
  }

  /********************************************************************************************************************/

  std::list<mtca4u::RegisterInfoMap::RegisterInfo> MemoryAddressedBackend::getRegistersInModule(
      const std::string &moduleName) const {
    return _registerMap->getRegistersInModule(moduleName);
  }

  /********************************************************************************************************************/

  std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > MemoryAddressedBackend::getRegisterAccessorsInModule(
      const std::string &moduleName) {

    std::list<RegisterInfoMap::RegisterInfo> registerInfoList =
        _registerMap->getRegistersInModule(moduleName);

    std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > accessorList;
    for (std::list<RegisterInfoMap::RegisterInfo>::const_iterator regInfo =
        registerInfoList.begin();
        regInfo != registerInfoList.end(); ++regInfo) {
      accessorList.push_back( boost::shared_ptr<mtca4u::RegisterAccessor>(
          new MemoryAddressedBackendRegisterAccessor(*regInfo, boost::static_pointer_cast<DeviceBackend>(shared_from_this()) )
      ) );
    }

    return accessorList;
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackend::checkRegister(const std::string &regName,
      const std::string &regModule, size_t dataSize,
      uint32_t addRegOffset, uint32_t &retDataSize,
      uint32_t &retRegOff, uint8_t &retRegBar) const {

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, regModule);
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4", DeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4", DeviceException::EX_WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.nBytes - addRegOffset) {
        throw DeviceException("Data size exceed register size", DeviceException::EX_WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.nBytes;
    }
    retRegBar = registerInfo.bar;
    retRegOff = registerInfo.address + addRegOffset;
  }

  /********************************************************************************************************************/

  /// Helper class to implement the templated getRegisterAccessor2D function with boost::fusion::for_each.
  /// This allows to avoid writing an if-statement for each supported user type.
  class getRegisterAccessor2DimplClass
  {
    public:
      getRegisterAccessor2DimplClass(const std::type_info &_type, const std::string &_dataRegionName,
          const std::string &_module, boost::shared_ptr<DeviceBackend> _backend)
      : type(_type), dataRegionName(_dataRegionName), module(_module), backend(_backend), ptr(NULL)
      {}

      /// This function will be called by for_each() for each type in FixedPointConverter::userTypeMap
      template <typename Pair>
      void operator()(Pair) const
      {
        // obtain UserType from given fusion::pair type
        typedef typename Pair::first_type ConvertedDataType;

        // check if UserType is the requested type
        if(typeid(ConvertedDataType) == type) {
          MixedTypeMuxedDataAccessor<ConvertedDataType> *typedptr;
          typedptr = new MixedTypeMuxedDataAccessor<ConvertedDataType>(dataRegionName,module,backend);
          ptr = static_cast<void*>(typedptr);
        }
      }

      /// Arguments passed from readImpl
      const std::type_info &type;
      const std::string &dataRegionName;
      const std::string &module;
      boost::shared_ptr<DeviceBackend> backend;

      /// pointer to the created accessor (or NULL if an invalid type was passed)
      mutable void *ptr;
  };

  /********************************************************************************************************************/

  void* MemoryAddressedBackend::getRegisterAccessor2Dimpl(const std::type_info &UserType, const std::string &dataRegionName,
      const std::string &module) {
    FixedPointConverter::userTypeMap userTypes;
    getRegisterAccessor2DimplClass impl(UserType,dataRegionName,module, boost::static_pointer_cast<DeviceBackend>(shared_from_this()) );
    boost::fusion::for_each(userTypes, impl);
    if(!impl.ptr) {
      throw DeviceException("Unknown user type passed to AddressBasedBackend::getRegisterAccessor2D()", DeviceException::EX_WRONG_PARAMETER);
    }
    return impl.ptr;
  }

} // namespace mtca4u
