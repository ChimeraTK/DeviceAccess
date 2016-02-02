/*
 * AddressBasedRegisterAccessor.cc
 *
 *  Created on: Feb 2, 2016
 *      Author: mhier
 */

#include "AddressBasedRegisterAccessor.h"
#include "AddressBasedBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  /********************************************************************************************************************/
  AddressBasedRegisterAccessor::AddressBasedRegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
        boost::shared_ptr<DeviceBackend> deviceBackendPointer)
  : RegisterAccessor(deviceBackendPointer),
    _registerInfo(registerInfo),
    _fixedPointConverter(_registerInfo.width, _registerInfo.nFractionalBits, _registerInfo.signedFlag)
  {}

  /********************************************************************************************************************/

  void AddressBasedRegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &registerInfo,
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

  void AddressBasedRegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->read(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void AddressBasedRegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->write(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void AddressBasedRegisterAccessor::readDMA(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    readRaw(data,dataSize,addRegOffset);
  }

  /********************************************************************************************************************/

  void AddressBasedRegisterAccessor::writeDMA(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    writeRaw(data,dataSize,addRegOffset);
  }

  /********************************************************************************************************************/

  RegisterInfoMap::RegisterInfo const &AddressBasedRegisterAccessor::getRegisterInfo() const {
    return _registerInfo;
  }

  /********************************************************************************************************************/

  FixedPointConverter const &AddressBasedRegisterAccessor::getFixedPointConverter() const {
    return _fixedPointConverter;
  }

} // namespace mtca4u
