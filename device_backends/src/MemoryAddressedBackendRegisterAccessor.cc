/*
 * AddressBasedRegisterAccessor.cc
 *
 *  Created on: Feb 2, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/fusion/algorithm.hpp>

#include "MemoryAddressedBackendRegisterAccessor.h"
#include "MemoryAddressedBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  /********************************************************************************************************************/
  MemoryAddressedBackendRegisterAccessor::MemoryAddressedBackendRegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
        boost::shared_ptr<DeviceBackend> deviceBackendPointer)
  : RegisterAccessor(deviceBackendPointer),
    _registerInfo(registerInfo),
    _fixedPointConverter(_registerInfo.width, _registerInfo.nFractionalBits, _registerInfo.signedFlag)
  {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(read_impl);
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(write_impl);
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &registerInfo,
      size_t dataSize,
      uint32_t addRegOffset,
      uint32_t &retDataSize,
      uint32_t &retRegOff) {
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4",
          DeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4",
            DeviceException::EX_WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.nBytes - addRegOffset) {
        throw DeviceException("Data size exceed register size",
            DeviceException::EX_WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.nBytes;
    }
    retRegOff = registerInfo.address + addRegOffset;
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->read(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->write(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  RegisterInfoMap::RegisterInfo const &MemoryAddressedBackendRegisterAccessor::getRegisterInfo() const {
    return _registerInfo;
  }

  /********************************************************************************************************************/

  FixedPointConverter const &MemoryAddressedBackendRegisterAccessor::getFixedPointConverter() const {
    return _fixedPointConverter;
  }

  /********************************************************************************************************************/

  unsigned int MemoryAddressedBackendRegisterAccessor::getNumberOfElements() const {
    return _registerInfo.nElements;
  }

  /********************************************************************************************************************/

  template <typename ConvertedDataType>
  void MemoryAddressedBackendRegisterAccessor::read_impl(ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const {
    std::vector<int32_t> rawDataBuffer(nWords);
    readRaw(&(rawDataBuffer[0]), nWords*sizeof(int32_t), wordOffsetInRegister*sizeof(int32_t));
    ConvertedDataType *data = reinterpret_cast<ConvertedDataType*>(convertedData);
    for(size_t i=0; i < nWords; ++i){
      data[i] = _fixedPointConverter.template toCooked<ConvertedDataType>(rawDataBuffer[i]);
    }
  }

  /********************************************************************************************************************/

  template <typename ConvertedDataType>
  void MemoryAddressedBackendRegisterAccessor::write_impl(const ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) {
    const ConvertedDataType *data = reinterpret_cast<const ConvertedDataType*>(convertedData);
    std::vector<int32_t> rawDataBuffer(nWords);
    for (size_t i = 0; i < nWords; ++i) {
      rawDataBuffer[i] = _fixedPointConverter.toRaw(data[i]);
    }
    writeRaw(&(rawDataBuffer[0]), nWords * sizeof(int32_t), wordOffsetInRegister * sizeof(int32_t));
  }

} // namespace mtca4u
