/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
      Device::DeviceBackendPointer deviceBackendPointer)
  : _registerInfo(registerInfo),
    _deviceBackendPointer(deviceBackendPointer),
    _fixedPointConverter(_registerInfo.width, _registerInfo.nFractionalBits, _registerInfo.signedFlag) {}

  void RegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &registerInfo,
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

  void RegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->read(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  void RegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->write(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  void RegisterAccessor::readDMA(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    readRaw(data,dataSize,addRegOffset);
  }

  void RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    writeRaw(data,dataSize,addRegOffset);
  }

  RegisterInfoMap::RegisterInfo const &RegisterAccessor::getRegisterInfo() const {
    return _registerInfo;
  }

  FixedPointConverter const &RegisterAccessor::getFixedPointConverter()
  const {
    return _fixedPointConverter;
  }

}
