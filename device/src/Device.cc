#include <cmath>
#include <string.h>

#include "Device.h"
#include "DeviceBackend.h"
#include "MapFileParser.h"
#include "RegisterAccessor.h"
#include "Utilities.h"

// for compatibility only:
#include "NumericAddress.h"
using ChimeraTK::numeric_address::BAR;

namespace ChimeraTK {

  /********************************************************************************************************************/

Device::Device(const std::string &aliasName) {
  BackendFactory &factoryInstance = BackendFactory::getInstance();
  _deviceBackendPointer = factoryInstance.createBackend(aliasName);
}

  /********************************************************************************************************************/

Device::~Device() {
  // Do NOT close the Backend here. Another device might be using the same
  // backend.
  }

  /********************************************************************************************************************/

  const RegisterCatalogue& Device::getRegisterCatalogue() const {
    checkPointersAreNotNull();
    return _deviceBackendPointer->getRegisterCatalogue();
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> Device::getRegisterMap() const {
    checkPointersAreNotNull();
    auto castedBackend = boost::dynamic_pointer_cast<NumericAddressedBackend>(_deviceBackendPointer);
    if(!castedBackend) {
      throw ChimeraTK::logic_error("Device::getRegisterMap() called for a "
                                   "non-NumericAddressedBackend. Use "
                                   "Device::getRegisterCatalogue() instead!");
    }
    return castedBackend->getRegisterMap();
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterAccessor> Device::getRegisterAccessor(const std::string& regName,
      const std::string& module) const {
    checkPointersAreNotNull();
    return boost::shared_ptr<RegisterAccessor>(
        new RegisterAccessor(_deviceBackendPointer, RegisterPath(module) / regName));
  }

  /********************************************************************************************************************/

  std::list<RegisterInfoMap::RegisterInfo> Device::getRegistersInModule(const std::string& moduleName) const {
    checkPointersAreNotNull();
    auto castedBackend = boost::dynamic_pointer_cast<NumericAddressedBackend>(_deviceBackendPointer);
    if(!castedBackend) {
      throw ChimeraTK::logic_error("Device::getRegistersInModule() called for a "
                                   "non-NumericAddressedBackend. Use "
                                   "Device::getRegisterCatalogue() instead!");
    }
    return castedBackend->getRegistersInModule(moduleName);
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

  /********************************************************************************************************************/

  std::list<boost::shared_ptr<ChimeraTK::RegisterAccessor>> Device::getRegisterAccessorsInModule(
      const std::string& moduleName) const {
    checkPointersAreNotNull();
    auto reglist = getRegistersInModule(moduleName);
    std::list<boost::shared_ptr<ChimeraTK::RegisterAccessor>> acclist;
    for(auto it = reglist.begin(); it != reglist.end(); ++it) {
      acclist.push_back(getRegisterAccessor(it->getRegisterName(), ""));
    }
    return acclist;
  }

  /********************************************************************************************************************/

  void Device::readReg(const std::string& regName, int32_t* data, size_t dataSize, uint32_t addRegOffset) const {
    readReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::readReg(const std::string& regName, const std::string& regModule, int32_t* data, size_t dataSize,
      uint32_t addRegOffset) const {
    if(dataSize % sizeof(int32_t) != 0) {
      throw ChimeraTK::logic_error("Wrong data size - must be dividable by 4");
    }
    if(addRegOffset % sizeof(int32_t) != 0) {
      throw ChimeraTK::logic_error("Wrong additional register offset - must be dividable by 4");
    }
    auto vec = read<int32_t>(
        RegisterPath(regModule) / regName, dataSize / sizeof(int32_t), addRegOffset / sizeof(int32_t), true);
    memcpy(data, vec.data(), vec.size() * sizeof(int32_t));
  }

  /********************************************************************************************************************/

  void Device::writeReg(const std::string& regName, int32_t const* data, size_t dataSize, uint32_t addRegOffset) {
    writeReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeReg(const std::string& regName, const std::string& regModule, int32_t const* data, size_t dataSize,
      uint32_t addRegOffset) {
    if(dataSize == 0) dataSize = sizeof(int32_t);
    if(dataSize % sizeof(int32_t) != 0) {
      throw ChimeraTK::logic_error("Wrong data size: - must be dividable by 4");
    }
    if(addRegOffset % sizeof(int32_t) != 0) {
      throw ChimeraTK::logic_error("Wrong additional register offset - must be dividable by 4");
    }
    std::vector<int32_t> vec(dataSize / sizeof(int32_t));
    memcpy(vec.data(), data, dataSize);
    write(RegisterPath(regModule) / regName, vec, addRegOffset / sizeof(int32_t), true);
  }

  /********************************************************************************************************************/

  void Device::readDMA(const std::string& regName, int32_t* data, size_t dataSize, uint32_t addRegOffset) const {
    readDMA(regName, std::string(), data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::readDMA(const std::string& regName, const std::string& regModule, int32_t* data, size_t dataSize,
      uint32_t addRegOffset) const {
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::readDMA() detected.    "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::read() instead!           "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    readReg(regName, regModule, data, dataSize, addRegOffset);
  }

  /********************************************************************************************************************/

  void Device::writeDMA(const std::string& regName, int32_t const* data, size_t dataSize, uint32_t addRegOffset) {
    writeDMA(regName, std::string(), data, dataSize, addRegOffset);
  } // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  void Device::writeDMA(const std::string& regName, const std::string& regModule, int32_t const* data, size_t dataSize,
      uint32_t addRegOffset) {
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::writeDMA() detected.   "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::write() instead!          "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    writeReg(regName, regModule, data, dataSize, addRegOffset);
  } // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  void Device::close() {
    checkPointersAreNotNull();
    _deviceBackendPointer->close();
  }

  /********************************************************************************************************************/

  void Device::readReg(uint32_t regOffset, int32_t* data, uint8_t bar) const {
    readReg(BAR / bar / regOffset * sizeof(int32_t), data, sizeof(int32_t), 0);
  }

  /********************************************************************************************************************/

  void Device::writeReg(uint32_t regOffset, int32_t data, uint8_t bar) {
    writeReg(BAR / bar / regOffset * sizeof(int32_t), &data, sizeof(int32_t), 0);
  }

  /********************************************************************************************************************/

  void Device::readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar) const {
    readReg(BAR / bar / regOffset * size, data, size, 0);
  }

  /********************************************************************************************************************/

  void Device::writeArea(uint32_t regOffset, int32_t const* data, size_t size, uint8_t bar) {
    writeReg(BAR / bar / regOffset * size, data, size, 0);
  }

  /********************************************************************************************************************/

  void Device::readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar) const {
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::readDMA() detected.    "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::read() instead!           "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl;               // LCOV_EXCL_LINE
    readArea(regOffset, data, size, bar); // LCOV_EXCL_LINE
  }                                       // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  void Device::writeDMA(uint32_t regOffset, int32_t const* data, size_t size, uint8_t bar) {
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::writeDMA() detected.   "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use register accessors or Device::write() instead!          "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl;                // LCOV_EXCL_LINE
    writeArea(regOffset, data, size, bar); // LCOV_EXCL_LINE
  }                                        // LCOV_EXCL_LINE
#pragma GCC diagnostic pop

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

  void Device::open(boost::shared_ptr<DeviceBackend> deviceBackend) {
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function detected.                      "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Signature:                                                  "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Device::open(boost::shared_ptr<DeviceBackend>)              "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "**                                                             "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use open() by alias name instead!                           "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    _deviceBackendPointer = deviceBackend;
    if(!_deviceBackendPointer->isOpen()) {
      _deviceBackendPointer->open();
    }
  }

  /********************************************************************************************************************/

  void Device::open() {
    checkPointersAreNotNull();
    if(!_deviceBackendPointer->isOpen()) {
      _deviceBackendPointer->open();
    }
  }

  /********************************************************************************************************************/

  void Device::open(std::string const& aliasName) {
    BackendFactory& factoryInstance = BackendFactory::getInstance();
    _deviceBackendPointer = factoryInstance.createBackend(aliasName);
    if(!_deviceBackendPointer->isOpen()) { // createBackend may return an already
                                           // opened instance for some backends
      _deviceBackendPointer->open();
    }
  }

  /********************************************************************************************************************/

  void Device::open(boost::shared_ptr<DeviceBackend>,
      boost::shared_ptr<ChimeraTK::RegisterInfoMap>&) { // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of removed function detected.                         "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Signature:                                                  "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Device::open(boost::shared_ptr<DeviceBackend>, "
                 "boost::shared_ptr<ChimeraTK::RegisterInfoMap>&) **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "**                                                             "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use open() by alias name instead!                           "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::terminate();
  } // LCOV_EXCL_LINE

  /********************************************************************************************************************/

  bool Device::isOpened() const {
    if(static_cast<bool>(_deviceBackendPointer) != false) {
      return _deviceBackendPointer->isOpen();
    }
    return false; // no backend is assigned: the device is not opened
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
