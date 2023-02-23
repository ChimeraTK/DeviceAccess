// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Device.h"

#include "DeviceBackend.h"

#include <cmath>
#include <string.h>

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
    if(static_cast<bool>(_deviceBackendPointer) == false) {
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
    if(static_cast<bool>(_deviceBackendPointer) != false) {
      return _deviceBackendPointer->isOpen();
    }
    return false; // no backend is assigned: the device is not opened
  }

  /********************************************************************************************************************/

  bool Device::isFunctional() const {
    if(static_cast<bool>(_deviceBackendPointer) != false) {
      std::cout<<"Device::isFunctional():"<< _deviceBackendPointer->isFunctional()<<std::endl;
      return _deviceBackendPointer->isFunctional();
    }
    std::cout<<"Device::isFunctional(): no backend is assigned: the device is not opened"<<std::endl;
    return false; // no backend is assigned: the device is not opened
  }

  /********************************************************************************************************************/

  void Device::activateAsyncRead() noexcept {
    _deviceBackendPointer->activateAsyncRead();
  }

  /********************************************************************************************************************/

  void Device::setException() {
    std::cout<<"Device::setException()"<<std::endl;
    _deviceBackendPointer->setException();
  }

  /********************************************************************************************************************/

  VoidRegisterAccessor Device::getVoidRegisterAccessor(
      const RegisterPath& registerPathName, const AccessModeFlags& flags) const {
    checkPointersAreNotNull();
    return VoidRegisterAccessor(_deviceBackendPointer->getRegisterAccessor<Void>(registerPathName, 0, 0, flags));
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> Device::getBackend() {
    return _deviceBackendPointer;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
