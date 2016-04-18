/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

#include <typeinfo>

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer,
      const RegisterPath &registerPathName)
    : _registerPathName(registerPathName), _backend(deviceBackendPointer),
    _registerInfo(_backend->getRegisterCatalogue().getRegister(registerPathName)){
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

}
