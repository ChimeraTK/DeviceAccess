/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer)
  : _deviceBackendPointer(deviceBackendPointer) {}

  RegisterAccessor::~RegisterAccessor() {}

  /********************************************************************************************************************/

  void RegisterAccessor::readDMA(int32_t *data, size_t dataSize, uint32_t addRegOffset) const {
    readRaw(data,dataSize,addRegOffset);
  }

  /********************************************************************************************************************/

  void RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize, uint32_t addRegOffset) {
    writeRaw(data,dataSize,addRegOffset);
  }

}
