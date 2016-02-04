#include <cmath>

#include "Device.h"
#include "RegisterAccessor.h"
#include "DeviceBackend.h"
#include "MapFileParser.h"
#include "Utilities.h"

namespace mtca4u{

  Device::~Device(){
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> Device::getRegisterMap() const {
    return _deviceBackendPointer->getRegisterMap();
  }

  /********************************************************************************************************************/

  boost::shared_ptr<Device::RegisterAccessor> Device::getRegisterAccessor(const std::string &regName,
      const std::string &module) const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getRegisterAccessor(regName, module);
  }

  /********************************************************************************************************************/

  std::list<RegisterInfoMap::RegisterInfo> Device::getRegistersInModule(
      const std::string &moduleName) const {
    checkPointersAreNotNull();

    return _deviceBackendPointer->getRegistersInModule(moduleName);
  }

  /********************************************************************************************************************/

  std::list< boost::shared_ptr<mtca4u::RegisterAccessor> >
  Device::getRegisterAccessorsInModule(const std::string &moduleName) const {
    checkPointersAreNotNull();

    return _deviceBackendPointer->getRegisterAccessorsInModule(moduleName);
  }

  /********************************************************************************************************************/

  void Device::readReg(const std::string &regName, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    readReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::readReg(const std::string &regName,
      const std::string &regModule, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    checkPointersAreNotNull();
    _deviceBackendPointer->read(regModule, regName, data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeReg(const std::string &regName, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    writeReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeReg(const std::string &regName,
      const std::string &regModule, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    checkPointersAreNotNull();
    _deviceBackendPointer->write(regModule, regName, data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::readDMA(const std::string &regName, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    readDMA(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::readDMA(const std::string &regName,
      const std::string &regModule, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    readReg(regName, regModule, data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeDMA(const std::string &regName, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    writeDMA(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeDMA(const std::string &regName,
      const std::string &regModule, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    writeReg(regName, regModule, data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::close() {
    checkPointersAreNotNull();
    _deviceBackendPointer->close();
  }

  /********************************************************************************************************************/

  void Device::readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const {
    checkPointersAreNotNull();
    _deviceBackendPointer->read(bar, regOffset, data , 4);
  }

  /********************************************************************************************************************/

  void Device::writeReg(uint32_t regOffset, int32_t data, uint8_t bar) {
    checkPointersAreNotNull();
    _deviceBackendPointer->write(bar, regOffset, &data, 4);
  }

  /********************************************************************************************************************/

  void Device::readArea(uint32_t regOffset, int32_t *data, size_t size,
      uint8_t bar) const {
    checkPointersAreNotNull();
    _deviceBackendPointer->read(bar, regOffset, data, size);
  }

  /********************************************************************************************************************/

  void Device::writeArea(uint32_t regOffset, int32_t const *data, size_t size,
      uint8_t bar) {
    checkPointersAreNotNull();
    _deviceBackendPointer->write(bar, regOffset, data, size);
  }

  /********************************************************************************************************************/

  void Device::readDMA(uint32_t regOffset, int32_t *data, size_t size,
      uint8_t bar) const {
    checkPointersAreNotNull();
    _deviceBackendPointer->read(bar, regOffset, data, size);
  }

  /********************************************************************************************************************/

  void Device::writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
      uint8_t bar) {
    checkPointersAreNotNull();
    _deviceBackendPointer->write(bar, regOffset, data, size);
  }

  /********************************************************************************************************************/

  std::string Device::readDeviceInfo() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->readDeviceInfo();
  }

  /********************************************************************************************************************/

  void Device::checkPointersAreNotNull() const {
    if ((_deviceBackendPointer == false)) {
      throw DeviceException("Device has not been opened correctly",
          DeviceException::EX_NOT_OPENED);
    }
  }

  /********************************************************************************************************************/

  void Device::open(boost::shared_ptr<DeviceBackend> deviceBackend)
  {
    _deviceBackendPointer = deviceBackend;
    _deviceBackendPointer->open();
  }

  /********************************************************************************************************************/

  void Device::open(std::string const & aliasName) {
    BackendFactory &factoryInstance = BackendFactory::getInstance();
    _deviceBackendPointer =  factoryInstance.createBackend(aliasName);
    try{
      _deviceBackendPointer->open();
    }catch(Exception &e){
      // The backend has already been allocated and probably opened.
      // Reset the pointer so the backend is closed and released.
      _deviceBackendPointer.reset();
      throw;
    }
  }

}// namespace mtca4u
