// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Device.h"

#include "DeviceBackend.h"

#include <cmath>
#include <cstring>

namespace ChimeraTK {

  /********************************************************************************************************************/

  Device::Device(const std::string& aliasName) {
    BackendFactory& factoryInstance = BackendFactory::getInstance();
    _deviceBackendPointer = factoryInstance.createBackend(aliasName);
  }

  /********************************************************************************************************************/

  RegisterCatalogue Device::getRegisterCatalogue() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getRegisterCatalogue();
  }

  /********************************************************************************************************************/

  MetadataCatalogue Device::getMetadataCatalogue() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getMetadataCatalogue();
  }

  /********************************************************************************************************************/

  std::string Device::readDeviceInfo() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->readDeviceInfo();
  }

  /********************************************************************************************************************/

  void Device::checkPointersAreNotNull() const {
    if(!static_cast<bool>(_deviceBackendPointer)) {
      throw ChimeraTK::logic_error("Device has not been opened correctly");
    }
  }

  /********************************************************************************************************************/

  void Device::open() {
    checkPointersAreNotNull();
    _deviceBackendPointer->open();
  }

  /********************************************************************************************************************/

  void Device::open(std::string const& aliasName) {
    BackendFactory& factoryInstance = BackendFactory::getInstance();
    _deviceBackendPointer = factoryInstance.createBackend(aliasName);
    _deviceBackendPointer->open();
  }

  /********************************************************************************************************************/

  void Device::close() {
    checkPointersAreNotNull();
    _deviceBackendPointer->close();
  }

  /********************************************************************************************************************/

  bool Device::isOpened() const {
    if(static_cast<bool>(_deviceBackendPointer)) {
      return _deviceBackendPointer->isOpen();
    }
    return false; // no backend is assigned: the device is not opened
  }

  /********************************************************************************************************************/

  bool Device::isFunctional() const {
    if(static_cast<bool>(_deviceBackendPointer)) {
      return _deviceBackendPointer->isFunctional();
    }
    return false; // no backend is assigned: the device is not opened
  }

  /********************************************************************************************************************/

  void Device::activateAsyncRead() noexcept {
    _deviceBackendPointer->activateAsyncRead();
  }

  /********************************************************************************************************************/

  void Device::setException(const std::string& message) {
    _deviceBackendPointer->setException(message);
  }

  /********************************************************************************************************************/

  VoidRegisterAccessor Device::getVoidRegisterAccessor(
      const RegisterPath& registerPathName, const AccessModeFlags& flags) const {
    checkPointersAreNotNull();
    return {_deviceBackendPointer->getRegisterAccessor<Void>(registerPathName, 0, 0, flags)};
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> Device::getBackend() {
    return _deviceBackendPointer;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
