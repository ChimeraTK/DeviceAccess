/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer,
      const RegisterPath &registerPathName)
  : _registerPathName(registerPathName), _dev(deviceBackendPointer){
    _intAccessor = _dev->getRegisterAccessor<int>(registerPathName);
  }

  RegisterAccessor::~RegisterAccessor() {}

  /********************************************************************************************************************/

  void RegisterAccessor::readDMA(int32_t *data, size_t dataSize, uint32_t addRegOffset) const {
    readRaw(data,dataSize,addRegOffset);
  }

  /********************************************************************************************************************/

  void RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize, uint32_t addRegOffset) {   // LCOV_EXCL_LINE
    writeRaw(data,dataSize,addRegOffset);   // LCOV_EXCL_LINE
  }   // LCOV_EXCL_LINE

  template <>
  void RegisterAccessor::read<int>(int *, size_t nWords = 1, uint32_t wordOffsetInRegister = 0) const{
  }


}
