/*
 * DeviceBackend.cc
 */


#include "DeviceBackend.h"
#include "RegisterAccessor.h"

namespace mtca4u {

  boost::shared_ptr<RegisterAccessor> AddressBasedBackend::getRegisterAccessor(const std::string &regName,
      const std::string &module) const {
    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, module);
    return boost::shared_ptr<Device::RegisterAccessor>(
        new Device::RegisterAccessor(registerInfo, shared_from_this()));
  }

}
