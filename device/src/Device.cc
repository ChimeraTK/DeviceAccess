#include "DummyBackend.h"
#include "Device.h"
#include "PcieBackend.h"
#include "DMapFilesParser.h"
#include "Utilities.h"
#include <cmath>
#include <boost/filesystem.hpp>


namespace mtca4u{

  Device::~Device(){
    // FIXME: do we want to close here? It will probably leave not working
    // RegisterAccessors
    // if(pdev) pdev->closeDev();
  }

  boost::shared_ptr<const RegisterInfoMap> Device::getRegisterMap() const {
    return _registerMap;
  }

  Device::RegisterAccessor Device::getRegObject(
      const std::string &regName) const {
    checkPointersAreNotNull();

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo);
    return Device::RegisterAccessor(registerInfo, _pDeviceBackend);
  }

  boost::shared_ptr<Device::RegisterAccessor> Device::getRegisterAccessor(const std::string &regName,
      const std::string &module) const {
    checkPointersAreNotNull();

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, module);
    return boost::shared_ptr<Device::RegisterAccessor>(
        new Device::RegisterAccessor(registerInfo, _pDeviceBackend));
  }

  std::list<RegisterInfoMap::RegisterInfo> Device::getRegistersInModule(
      const std::string &moduleName) const {
    checkPointersAreNotNull();

    return _registerMap->getRegistersInModule(moduleName);
  }

  std::list<typename Device::RegisterAccessor>
  Device::getRegisterAccessorsInModule(const std::string &moduleName) const {
    checkPointersAreNotNull();

    std::list<RegisterInfoMap::RegisterInfo> registerInfoList =
        _registerMap->getRegistersInModule(moduleName);

    std::list<RegisterAccessor> accessorList;
    for (std::list<RegisterInfoMap::RegisterInfo>::const_iterator regInfo =
        registerInfoList.begin();
        regInfo != registerInfoList.end(); ++regInfo) {
      accessorList.push_back(
          Device::RegisterAccessor(*regInfo, _pDeviceBackend));
    }

    return accessorList;
  }

  void Device::checkRegister(const std::string &regName,
      const std::string &regModule, size_t dataSize,
      uint32_t addRegOffset, uint32_t &retDataSize,
      uint32_t &retRegOff, uint8_t &retRegBar) const {
    checkPointersAreNotNull();

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, regModule);
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4",
          DeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4",
            DeviceException::EX_WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.reg_size - addRegOffset) {
        throw DeviceException("Data size exceed register size",
            DeviceException::EX_WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.reg_size;
    }
    retRegBar = registerInfo.reg_bar;
    retRegOff = registerInfo.reg_address + addRegOffset;
  }

  void Device::readReg(const std::string &regName, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    readReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  void Device::readReg(const std::string &regName,
      const std::string &regModule, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    readArea(retRegOff, data, retDataSize, retRegBar);
  }

  void Device::writeReg(const std::string &regName, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    writeReg(regName, std::string(), data, dataSize, addRegOffset);
  }

  void Device::writeReg(const std::string &regName,
      const std::string &regModule, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    writeArea(retRegOff, data, retDataSize, retRegBar);
  }

  void Device::readDMA(const std::string &regName, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    readDMA(regName, std::string(), data, dataSize, addRegOffset);
  }

  void Device::readDMA(const std::string &regName,
      const std::string &regModule, int32_t *data,
      size_t dataSize, uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    if (retRegBar != 0xD) {
      throw DeviceException("Cannot read data from register \"" + regName +
          "\" through DMA",
          DeviceException::EX_WRONG_PARAMETER);
    }
    readDMA(retRegOff, data, retDataSize, retRegBar);
  }

  void Device::writeDMA(const std::string &regName, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    writeDMA(regName, std::string(), data, dataSize, addRegOffset);
  }

  void Device::writeDMA(const std::string &regName,
      const std::string &regModule, int32_t const *data,
      size_t dataSize, uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;
    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    if (retRegBar != 0xD) {
      throw DeviceException("Cannot read data from register \"" + regName +
          "\" through DMA",
          DeviceException::EX_WRONG_PARAMETER);
    }
    writeDMA(retRegOff, data, retDataSize, retRegBar);
  }

  void Device::close() {
    checkPointersAreNotNull();
    _pDeviceBackend->close();
  }

  /**
   *      @brief  Function allows to read data from one register located in
   *              device address space
   *
   *      This is wrapper to standard readRaw function defined in libdev
   *library.
   *      Allows to read one register located in device address space. Size of
   *register
   *      depends on type of accessed device e.x. for PCIe device it is equal to
   *      32bit. Function throws the same exceptions like readRaw from class
   *type.
   *
   *
   *      @param  regOffset - offset of the register in device address space
   *      @param  data - pointer to area to store data
   *      @param  bar  - number of PCIe bar
   */

  void Device::readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const {
    checkPointersAreNotNull();
    //pdev->readReg(regOffset, data, bar);
    _pDeviceBackend->read(bar, regOffset, data , 4);
  }

  /**
   *      @brief  Function allows to write data to one register located in
   *              device address space
   *
   *      This is wrapper to standard writeRaw function defined in libdev
   *library.
   *      Allows to write one register located in device address space. Size of
   *register
   *      depends on type of accessed device e.x. for PCIe device it is equal to
   *      32bit. Function throws the same exceptions like writeRaw from class
   *type.
   *
   *      @param  regOffset - offset of the register in device address space
   *      @param  data - pointer to data to write
   *      @param  bar  - number of PCIe bar
   */

  void Device::writeReg(uint32_t regOffset, int32_t data, uint8_t bar) {
    checkPointersAreNotNull();
    _pDeviceBackend->write(bar, regOffset, &data, 4);
  }

  /**
   *      @brief  Function allows to read data from several registers located in
   *              device address space
   *
   *      This is wrapper to standard readArea function defined in libdev
   *library.
   *      Allows to read several registers located in device address space.
   *      Function throws the same exceptions like readArea from class type.
   *
   *
   *      @param  regOffset - offset of the register in device address space
   *      @param  data - pointer to area to store data
   *      @param  size - number of bytes to read from device
   *      @param  bar  - number of PCIe bar
   */

  void Device::readArea(uint32_t regOffset, int32_t *data, size_t size,
      uint8_t bar) const {
    checkPointersAreNotNull();
    //pdev->readArea(regOffset, data, size, bar);
    _pDeviceBackend->read(bar, regOffset, data, size);
  }

  void Device::writeArea(uint32_t regOffset, int32_t const *data, size_t size,
      uint8_t bar) {
    checkPointersAreNotNull();
    _pDeviceBackend->write(bar, regOffset, data, size);
  }

  void Device::readDMA(uint32_t regOffset, int32_t *data, size_t size,
      uint8_t bar) const {
    checkPointersAreNotNull();
    _pDeviceBackend->read(bar, regOffset, data, size);
  }

  void Device::writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
      uint8_t bar) {
    checkPointersAreNotNull();
    _pDeviceBackend->write(bar, regOffset, data, size);
  }

  std::string Device::readDeviceInfo() const {
    checkPointersAreNotNull();
    return _pDeviceBackend->readDeviceInfo();
  }

  Device::RegisterAccessor::RegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
      _ptrDeviceBackend pDeviceBackend)
  : _registerInfo(registerInfo),
    _pDeviceBackend(pDeviceBackend),
    _fixedPointConverter(_registerInfo.reg_width, _registerInfo.reg_frac_bits, _registerInfo.reg_signed) {}

  void Device::RegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &registerInfo,
      size_t dataSize,
      uint32_t addRegOffset,
      uint32_t &retDataSize,
      uint32_t &retRegOff) {
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4",
          DeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4",
            DeviceException::EX_WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.reg_size - addRegOffset) {
        throw DeviceException("Data size exceed register size",
            DeviceException::EX_WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.reg_size;
    }
    retRegOff = registerInfo.reg_address + addRegOffset;
  }

  void Device::RegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _pDeviceBackend->read(_registerInfo.reg_bar, retRegOff, data, retDataSize);
  }

  void Device::RegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _pDeviceBackend->write(_registerInfo.reg_bar, retRegOff, data, retDataSize);
  }

  void Device::RegisterAccessor::readDMA(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    readRaw(data,dataSize,addRegOffset);
  }

  void Device::RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    writeRaw(data,dataSize,addRegOffset);
  }

  RegisterInfoMap::RegisterInfo const &Device::RegisterAccessor::getRegisterInfo() const {
    return _registerInfo; // registerInfo is the RegisterInfoent
  }

  FixedPointConverter const &Device::RegisterAccessor::getFixedPointConverter()
  const {
    return _fixedPointConverter;
  }

  void Device::checkPointersAreNotNull() const {
    if ((_pDeviceBackend == false) || (_registerMap == false)) {
      throw DeviceException("Device has not been opened correctly",
          DeviceException::EX_NOT_OPENED);
    }
  }

  void Device::open(boost::shared_ptr<DeviceBackend> DeviceBackend,
      boost::shared_ptr<RegisterInfoMap> registerInfoMap)
  {
    _pDeviceBackend = DeviceBackend;
    _pDeviceBackend->open();
    _registerMap = registerInfoMap;
  }

void Device::open(std::string const & aliasName) {
	BackendFactory FactoryInstance = BackendFactory::getInstance();
	_pDeviceBackend =  FactoryInstance.createBackend(aliasName);
	if (_pDeviceBackend)
		_pDeviceBackend->open();
	else
		return;
	//find the file containing first occurance of alias name.
  std::string dmapfile = Utilities::findFirstOfAlias(aliasName);
  DMapFilesParser filesParser;
  try
  {
    if ( boost::filesystem::exists(dmapfile ) )
    {
      filesParser.parse_file(dmapfile);
    }
  }
  catch (Exception& e) {
    std::cout << e.what() << std::endl;
  }
  DMapFile::DRegisterInfo DRegisterInfoent;
  for (DMapFilesParser::iterator deviceIter = filesParser.begin();
      deviceIter != filesParser.end(); ++deviceIter) {
    if (boost::iequals((*deviceIter).first.dev_name, aliasName)) {
      DRegisterInfoent = (*deviceIter).first;
      _mapFileName = DRegisterInfoent.map_file_name;
      _registerMap = mtca4u::MapFileParser().parse(_mapFileName);
      break;
    }
  }
}


}// namespace mtca4u
