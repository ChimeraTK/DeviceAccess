/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer)
  : _deviceBackendPointer(deviceBackendPointer) {}

  RegisterAccessor::~RegisterAccessor() {}

}
