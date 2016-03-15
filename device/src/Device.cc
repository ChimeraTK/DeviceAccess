#include <string.h>
#include <cmath>

#include "Device.h"
#include "RegisterAccessor.h"
#include "DeviceBackend.h"
#include "MapFileParser.h"
#include "Utilities.h"

namespace mtca4u {

  Device::~Device() {
    if(_deviceBackendPointer != false) {        // Some backend is assigned: close it if opened.
      if(_deviceBackendPointer->isOpen()) {     // This prevents any existing register accessor of this device to be
        _deviceBackendPointer->close();         // used after this device was assigned to a new backend.
      }
    }
  }

  /********************************************************************************************************************/

  const RegisterCatalogue& Device::getRegisterCatalogue() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getRegisterCatalogue();
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> Device::getRegisterMap() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getRegisterMap();
  }

  /********************************************************************************************************************/

  boost::shared_ptr<Device::RegisterAccessor> Device::getRegisterAccessor(const std::string &regName,
      const std::string &module) const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getRegisterAccessor(regName, module);
  }

  /********************************************************************************************************************/

  std::list<RegisterInfoMap::RegisterInfo> Device::getRegistersInModule(const std::string &moduleName) const {
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
    if(dataSize % sizeof(int32_t) != 0) {
      throw DeviceException("Wrong data size - must be dividable by 4", DeviceException::EX_WRONG_PARAMETER);
    }
    if(addRegOffset % sizeof(int32_t) != 0) {
      throw DeviceException("Wrong additional register offset - must be dividable by 4", DeviceException::EX_WRONG_PARAMETER);
    }
    try {
      auto vec = read<int32_t>(RegisterPath(regModule)/regName, dataSize/sizeof(int32_t), addRegOffset/sizeof(int32_t), true);
      memcpy(data,vec.data(),vec.size()*sizeof(int32_t));
    }
    catch(DeviceException &e) { // translate exception for compatibility
      if(e.getID() == DeviceException::REGISTER_DOES_NOT_EXIST) {
        throw mtca4u::MapFileException(e.what(), mtca4u::LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
      }
      throw;
    }
  }

  /********************************************************************************************************************/

  void Device::writeReg(const std::string &regName, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    writeReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeReg(const std::string &regName, const std::string &regModule, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    if(dataSize == 0) dataSize = sizeof(int32_t);
    if(dataSize % sizeof(int32_t) != 0) {
      throw DeviceException("Wrong data size: - must be dividable by 4", DeviceException::EX_WRONG_PARAMETER);
    }
    if(addRegOffset % sizeof(int32_t) != 0) {
      throw DeviceException("Wrong additional register offset - must be dividable by 4", DeviceException::EX_WRONG_PARAMETER);
    }
    std::vector<int32_t> vec(dataSize/sizeof(int32_t));
    memcpy(vec.data(),data,dataSize);
    write(RegisterPath(regModule)/regName, vec, addRegOffset/sizeof(int32_t), true);
  }

  /********************************************************************************************************************/

  void Device::readDMA(const std::string &regName, int32_t *data, size_t dataSize, uint32_t addRegOffset) const {
    readDMA(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::readDMA(const std::string &regName, const std::string &regModule, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::readDMA() detected.                                    **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::read() instead!                                           **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    readReg(regName, regModule, data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeDMA(const std::string &regName, int32_t const *data, size_t dataSize, uint32_t addRegOffset) {
    writeDMA(regName, std::string(), data, dataSize, addRegOffset);
  }   // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  void Device::writeDMA(const std::string &regName, const std::string &regModule, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::writeDMA() detected.                                   **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::write() instead!                                          **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    writeReg(regName, regModule, data, dataSize, addRegOffset);
  }   // LCOV_EXCL_LINE

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

  void Device::readArea(uint32_t regOffset, int32_t *data, size_t size, uint8_t bar) const {
    checkPointersAreNotNull();
    _deviceBackendPointer->read(bar, regOffset, data, size);
  }

  /********************************************************************************************************************/

  void Device::writeArea(uint32_t regOffset, int32_t const *data, size_t size, uint8_t bar) {
    checkPointersAreNotNull();
    _deviceBackendPointer->write(bar, regOffset, data, size);
  }

  /********************************************************************************************************************/

  void Device::readDMA(uint32_t regOffset, int32_t *data, size_t size, uint8_t bar) const {
    checkPointersAreNotNull();// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::readDMA() detected.                                    **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::read() instead!                                           **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    _deviceBackendPointer->read(bar, regOffset, data, size);    // LCOV_EXCL_LINE
  }                                                             // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  void Device::writeDMA(uint32_t regOffset, int32_t const *data, size_t size, uint8_t bar) {
    checkPointersAreNotNull();// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::writeDMA() detected.                                   **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::write() instead!                                          **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    _deviceBackendPointer->write(bar, regOffset, data, size);   // LCOV_EXCL_LINE
  }                                                             // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  std::string Device::readDeviceInfo() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->readDeviceInfo();
  }

  /********************************************************************************************************************/

  void Device::checkPointersAreNotNull() const {
    if ((_deviceBackendPointer == false)) {
      throw DeviceException("Device has not been opened correctly", DeviceException::EX_NOT_OPENED);
    }
  }

  /********************************************************************************************************************/

  void Device::open(boost::shared_ptr<DeviceBackend> deviceBackend)
  {
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function detected.                                                      **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Signature:                                                                                  **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Device::open(boost::shared_ptr<DeviceBackend>)                                              **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "**                                                                                             **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use open() by alias name instead!                                                           **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    _deviceBackendPointer = deviceBackend;
    _deviceBackendPointer->open();
  }

  /********************************************************************************************************************/

  void Device::open() {
    checkPointersAreNotNull();
    _deviceBackendPointer->open();
  }

  /********************************************************************************************************************/

  void Device::open(std::string const & aliasName) {
    BackendFactory &factoryInstance = BackendFactory::getInstance();
    if(_deviceBackendPointer != false) {        // Some backend is already assigned: close it if opened.
      if(_deviceBackendPointer->isOpen()) {     // This prevents any existing register accessor of this device to be
        _deviceBackendPointer->close();         // used after this device was assigned to a new backend.
      }
    }
    _deviceBackendPointer =  factoryInstance.createBackend(aliasName);
    _deviceBackendPointer->open();
  }

  /********************************************************************************************************************/

  void Device::open(boost::shared_ptr<DeviceBackend> deviceBackend, boost::shared_ptr<mtca4u::RegisterInfoMap> &registerMap) {// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function detected.                                                      **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Signature:                                                                                  **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Device::open(boost::shared_ptr<DeviceBackend>, boost::shared_ptr<mtca4u::RegisterInfoMap>&) **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "**                                                                                             **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use open() by alias name instead!                                                           **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    deviceBackend->setRegisterMap(registerMap);// LCOV_EXCL_LINE
    open(deviceBackend);// LCOV_EXCL_LINE
  }// LCOV_EXCL_LINE

}// namespace mtca4u
