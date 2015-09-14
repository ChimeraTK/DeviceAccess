#include "FakeBackend.h"
#include "DummyBackend.h"
#include "Device.h"
#include "PcieBackend.h"
#include <cmath>


namespace mtca4u{


 Device::~Device(){
	// FIXME: do we want to close here? It will probably leave not working
	// RegisterAccessors
	// if(pdev) pdev->closeDev();
}



boost::shared_ptr<const RegisterInfoMap> Device::getRegisterMap() const {
	return registerMap;
}


Device::RegisterAccessor Device::getRegObject(
		const std::string &regName) const {
	checkPointersAreNotNull();

	RegisterInfoMap::RegisterInfo me;
	registerMap->getRegisterInfo(regName, me);
	return Device::RegisterAccessor(regName, me, pdev);
}


boost::shared_ptr<Device::RegisterAccessor> Device::getRegisterAccessor(const std::string &regName,
		const std::string &module) const {
	checkPointersAreNotNull();

	RegisterInfoMap::RegisterInfo me;
	registerMap->getRegisterInfo(regName, me, module);
	//return boost::shared_ptr<typename Device::RegisterAccessor>(
	return boost::shared_ptr<Device::RegisterAccessor>(
			new Device::RegisterAccessor(regName, me, pdev));
}



std::list<RegisterInfoMap::RegisterInfo> Device::getRegistersInModule(
		const std::string &moduleName) const {
	checkPointersAreNotNull();

	return registerMap->getRegistersInModule(moduleName);
}


std::list<typename Device::RegisterAccessor>
Device::getRegisterAccessorsInModule(const std::string &moduleName) const {
	checkPointersAreNotNull();

	std::list<RegisterInfoMap::RegisterInfo> registerInfoList =
			registerMap->getRegistersInModule(moduleName);

	std::list<RegisterAccessor> accessorList;
	for (std::list<RegisterInfoMap::RegisterInfo>::const_iterator regInfo =
			registerInfoList.begin();
			regInfo != registerInfoList.end(); ++regInfo) {
		accessorList.push_back(
				Device::RegisterAccessor(regInfo->reg_name, *regInfo, pdev));
	}

	return accessorList;
}


void Device::checkRegister(const std::string &regName,
		const std::string &regModule, size_t dataSize,
		uint32_t addRegOffset, uint32_t &retDataSize,
		uint32_t &retRegOff, uint8_t &retRegBar) const {
	checkPointersAreNotNull();

	RegisterInfoMap::RegisterInfo me;
	registerMap->getRegisterInfo(regName, me, regModule);
	if (addRegOffset % 4) {
		throw DeviceException("Register offset must be divisible by 4",
				DeviceException::EX_WRONG_PARAMETER);
	}
	if (dataSize) {
		if (dataSize % 4) {
			throw DeviceException("Data size must be divisible by 4",
					DeviceException::EX_WRONG_PARAMETER);
		}
		if (dataSize > me.reg_size - addRegOffset) {
			throw DeviceException("Data size exceed register size",
					DeviceException::EX_WRONG_PARAMETER);
		}
		retDataSize = dataSize;
	} else {
		retDataSize = me.reg_size;
	}
	retRegBar = me.reg_bar;
	retRegOff = me.reg_address + addRegOffset;
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
	pdev->close();
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
	pdev->read(bar, regOffset, data , 4);
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
	pdev->write(bar, regOffset, &data, 4);
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
	pdev->read(bar, regOffset, data, size);
}


void Device::writeArea(uint32_t regOffset, int32_t const *data, size_t size,
		uint8_t bar) {
	checkPointersAreNotNull();
	pdev->write(bar, regOffset, data, size);
}


void Device::readDMA(uint32_t regOffset, int32_t *data, size_t size,
		uint8_t bar) const {
	checkPointersAreNotNull();
	pdev->readDMA(bar, regOffset, data, size);
}


void Device::writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
		uint8_t bar) {
	checkPointersAreNotNull();
	pdev->writeDMA(bar, regOffset, data, size);
}


std::string Device::readDeviceInfo() const {
	checkPointersAreNotNull();
	return pdev->readDeviceInfo();
}


Device::RegisterAccessor::RegisterAccessor(const std::string & /*_regName*/,
		const RegisterInfoMap::RegisterInfo &_me,
		ptrdev _pdev)
: me(_me),
	pdev(_pdev),
	_fixedPointConverter(_me.reg_width, _me.reg_frac_bits, _me.reg_signed) {}


void Device::RegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &me,
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
		if (dataSize > me.reg_size - addRegOffset) {
			throw DeviceException("Data size exceed register size",
					DeviceException::EX_WRONG_PARAMETER);
		}
		retDataSize = dataSize;
	} else {
		retDataSize = me.reg_size;
	}
	retRegOff = me.reg_address + addRegOffset;
}


void Device::RegisterAccessor::readRaw(int32_t *data, size_t dataSize,
		uint32_t addRegOffset) const {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	pdev->read(me.reg_bar, retRegOff, data, retDataSize);
}


void Device::RegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
		uint32_t addRegOffset) {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	pdev->write(me.reg_bar, retRegOff, data, retDataSize);
}


void Device::RegisterAccessor::readDMA(int32_t *data, size_t dataSize,
		uint32_t addRegOffset) const {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	if (me.reg_bar != 0xD) {
		throw DeviceException("Cannot read data from register \"" + me.reg_name +
				"\" through DMA",
				DeviceException::EX_WRONG_PARAMETER);
	}
	pdev->readDMA(me.reg_bar, retRegOff, data, retDataSize);
}


void Device::RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize,
		uint32_t addRegOffset) {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	if (me.reg_bar != 0xD) {
		throw DeviceException("Cannot read data from register \"" + me.reg_name +
				"\" through DMA",
				DeviceException::EX_WRONG_PARAMETER);
	}
	pdev->writeDMA(me.reg_bar, retRegOff, data, retDataSize);
}


RegisterInfoMap::RegisterInfo const &Device::RegisterAccessor::getRegisterInfo() const {
	return me; // me is the RegisterInfoent
}


FixedPointConverter const &Device::RegisterAccessor::getFixedPointConverter()
const {
	return _fixedPointConverter;
}



void Device::checkPointersAreNotNull() const {
	if ((pdev == false) || (registerMap == false)) {
		throw DeviceException("Device has not been opened correctly",
				DeviceException::EX_NOT_OPENED);
	}
}





void Device::open(boost::shared_ptr<DeviceBackend> DeviceBackend,
		boost::shared_ptr<RegisterInfoMap> registerInfoMap)
{
	pdev = DeviceBackend;
	pdev->open();
	registerMap = registerInfoMap;
}

void Device::open(std::string const & aliasName) {
	BackendFactory FactoryInstance = BackendFactory::getInstance();
	pdev =  FactoryInstance.createDevice(aliasName);
	if (pdev)
		pdev->open();
	else
		return;
	DMapFilesParser filesParser;
	std::string dmapFileName = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
	try
	{
		if ( boost::filesystem::exists(dmapFileName ) )
		{
			filesParser.parse_file(dmapFileName);
		}
		else
		{
			dmapFileName = DMAP_FILE_PATH;
			filesParser.parse_file(DMAP_FILE_PATH);
		}
	}
	catch (Exception& e) {
		std::cout << e.what() << std::endl;
	}

	//DMapFile::dRegisterInfo registerInfo = filesParser.getdMapFileElem(aliasName);
	DMapFile::dRegisterInfo dRegisterInfoent;
	for (DMapFilesParser::iterator deviceIter = filesParser.begin();
			deviceIter != filesParser.end(); ++deviceIter) {
		if (boost::iequals((*deviceIter).first.dev_name, aliasName)) {
			//std::cout << "found:" << (*deviceIter).first.dev_file << std::endl;
			dRegisterInfoent = (*deviceIter).first;
			mapFileName = dRegisterInfoent.map_file_name;
			registerMap = mtca4u::mapFileParser().parse(mapFileName);
			break;
		}
	}
}

}// namespace mtca4u
