#include "FakeDevice.h"
#include "DummyDevice.h"
#include <cmath>

#include "MappedDevice.h"
#include "PcieDevice.h"

// Not so nice: macro code does not have code coverage report. Make sure you test everything!
// Check that nWords is not 0. The readReg command would read the whole register, which
// will the raw buffer size of 0.
#define MTCA4U_DEVMAP_READ( DEVICE_TYPE, CONVERTED_DATA_TYPE, ROUNDING_COMMAND ) \
		template<>\
		void MappedDevice::RegisterAccessor::read(CONVERTED_DATA_TYPE * convertedData, size_t nWords,\
				uint32_t wordOffsetInRegister) const { \
	if (nWords==0){\
		return;\
	}\
	\
	std::vector<int32_t> rawDataBuffer(nWords);\
	readRaw(&(rawDataBuffer[0]), nWords*sizeof(int32_t), wordOffsetInRegister*sizeof(int32_t));\
	\
	for(size_t i=0; i < nWords; ++i){\
		convertedData[i] = static_cast<CONVERTED_DATA_TYPE>(ROUNDING_COMMAND(_fixedPointConverter.toDouble(rawDataBuffer[i]))); \
	}\
}

//Trick for floating point types: Empty arguments are not allowed in C++98 (only from C++11).
//Just insert another static cast, double casting to the same type should be a no-op and optimised by the compiler.
#define MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( DEVICE_TYPE  )\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, int32_t, round )  \
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, uint32_t, round  )\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, int16_t, round  )\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, uint16_t, round  )\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, int8_t, round )\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, uint8_t, round  )\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, float , static_cast<float>)\
		MTCA4U_DEVMAP_READ( DEVICE_TYPE, double, static_cast<double> )\


namespace mtca4u{
MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( BaseDevice )


 MappedDevice::~MappedDevice(){
	// FIXME: do we want to close here? It will probably leave not working
	// RegisterAccessors
	// if(pdev) pdev->closeDev();
}



boost::shared_ptr<const RegisterInfoMap> MappedDevice::getRegisterMap() const {
	return registerMap;
}


MappedDevice::RegisterAccessor MappedDevice::getRegObject(
		const std::string &regName) const {
	checkPointersAreNotNull();

	RegisterInfoMap::RegisterInfo me;
	registerMap->getRegisterInfo(regName, me);
	return MappedDevice::RegisterAccessor(regName, me, pdev);
}


boost::shared_ptr<MappedDevice::RegisterAccessor> MappedDevice::getRegisterAccessor(const std::string &regName,
		const std::string &module) const {
	checkPointersAreNotNull();

	RegisterInfoMap::RegisterInfo me;
	registerMap->getRegisterInfo(regName, me, module);
	//return boost::shared_ptr<typename MappedDevice::RegisterAccessor>(
	return boost::shared_ptr<MappedDevice::RegisterAccessor>(
			new MappedDevice::RegisterAccessor(regName, me, pdev));
}



std::list<RegisterInfoMap::RegisterInfo> MappedDevice::getRegistersInModule(
		const std::string &moduleName) const {
	checkPointersAreNotNull();

	return registerMap->getRegistersInModule(moduleName);
}


std::list<typename MappedDevice::RegisterAccessor>
MappedDevice::getRegisterAccessorsInModule(const std::string &moduleName) const {
	checkPointersAreNotNull();

	std::list<RegisterInfoMap::RegisterInfo> registerInfoList =
			registerMap->getRegistersInModule(moduleName);

	std::list<RegisterAccessor> accessorList;
	for (std::list<RegisterInfoMap::RegisterInfo>::const_iterator regInfo =
			registerInfoList.begin();
			regInfo != registerInfoList.end(); ++regInfo) {
		accessorList.push_back(
				MappedDevice::RegisterAccessor(regInfo->reg_name, *regInfo, pdev));
	}

	return accessorList;
}


void MappedDevice::checkRegister(const std::string &regName,
		const std::string &regModule, size_t dataSize,
		uint32_t addRegOffset, uint32_t &retDataSize,
		uint32_t &retRegOff, uint8_t &retRegBar) const {
	checkPointersAreNotNull();

	RegisterInfoMap::RegisterInfo me;
	registerMap->getRegisterInfo(regName, me, regModule);
	if (addRegOffset % 4) {
		throw MappedDeviceException("Register offset must be divisible by 4",
				MappedDeviceException::EX_WRONG_PARAMETER);
	}
	if (dataSize) {
		if (dataSize % 4) {
			throw MappedDeviceException("Data size must be divisible by 4",
					MappedDeviceException::EX_WRONG_PARAMETER);
		}
		if (dataSize > me.reg_size - addRegOffset) {
			throw MappedDeviceException("Data size exceed register size",
					MappedDeviceException::EX_WRONG_PARAMETER);
		}
		retDataSize = dataSize;
	} else {
		retDataSize = me.reg_size;
	}
	retRegBar = me.reg_bar;
	retRegOff = me.reg_address + addRegOffset;
}


void MappedDevice::readReg(const std::string &regName, int32_t *data,
		size_t dataSize, uint32_t addRegOffset) const {
	readReg(regName, std::string(), data, dataSize, addRegOffset);
}


void MappedDevice::readReg(const std::string &regName,
		const std::string &regModule, int32_t *data,
		size_t dataSize, uint32_t addRegOffset) const {
	uint32_t retDataSize;
	uint32_t retRegOff;
	uint8_t retRegBar;

	checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
			retRegOff, retRegBar);
	readArea(retRegOff, data, retDataSize, retRegBar);
}


void MappedDevice::writeReg(const std::string &regName, int32_t const *data,
		size_t dataSize, uint32_t addRegOffset) {
	writeReg(regName, std::string(), data, dataSize, addRegOffset);
}


void MappedDevice::writeReg(const std::string &regName,
		const std::string &regModule, int32_t const *data,
		size_t dataSize, uint32_t addRegOffset) {
	uint32_t retDataSize;
	uint32_t retRegOff;
	uint8_t retRegBar;

	checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
			retRegOff, retRegBar);
	writeArea(retRegOff, data, retDataSize, retRegBar);
}


void MappedDevice::readDMA(const std::string &regName, int32_t *data,
		size_t dataSize, uint32_t addRegOffset) const {
	readDMA(regName, std::string(), data, dataSize, addRegOffset);
}


void MappedDevice::readDMA(const std::string &regName,
		const std::string &regModule, int32_t *data,
		size_t dataSize, uint32_t addRegOffset) const {
	uint32_t retDataSize;
	uint32_t retRegOff;
	uint8_t retRegBar;

	checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
			retRegOff, retRegBar);
	if (retRegBar != 0xD) {
		throw MappedDeviceException("Cannot read data from register \"" + regName +
				"\" through DMA",
				MappedDeviceException::EX_WRONG_PARAMETER);
	}
	readDMA(retRegOff, data, retDataSize, retRegBar);
}


void MappedDevice::writeDMA(const std::string &regName, int32_t const *data,
		size_t dataSize, uint32_t addRegOffset) {
	writeDMA(regName, std::string(), data, dataSize, addRegOffset);
}


void MappedDevice::writeDMA(const std::string &regName,
		const std::string &regModule, int32_t const *data,
		size_t dataSize, uint32_t addRegOffset) {
	uint32_t retDataSize;
	uint32_t retRegOff;
	uint8_t retRegBar;
	checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
			retRegOff, retRegBar);
	if (retRegBar != 0xD) {
		throw MappedDeviceException("Cannot read data from register \"" + regName +
				"\" through DMA",
				MappedDeviceException::EX_WRONG_PARAMETER);
	}
	writeDMA(retRegOff, data, retDataSize, retRegBar);
}


void MappedDevice::close() {
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

void MappedDevice::readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const {
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

void MappedDevice::writeReg(uint32_t regOffset, int32_t data, uint8_t bar) {
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

void MappedDevice::readArea(uint32_t regOffset, int32_t *data, size_t size,
		uint8_t bar) const {
	checkPointersAreNotNull();
	//pdev->readArea(regOffset, data, size, bar);
	pdev->read(bar, regOffset, data, size);
}


void MappedDevice::writeArea(uint32_t regOffset, int32_t const *data, size_t size,
		uint8_t bar) {
	checkPointersAreNotNull();
	pdev->write(bar, regOffset, data, size);
}


void MappedDevice::readDMA(uint32_t regOffset, int32_t *data, size_t size,
		uint8_t bar) const {
	checkPointersAreNotNull();
	pdev->readDMA(bar, regOffset, data, size);
}


void MappedDevice::writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
		uint8_t bar) {
	checkPointersAreNotNull();
	pdev->writeDMA(bar, regOffset, data, size);
}


std::string MappedDevice::readDeviceInfo() const {
	checkPointersAreNotNull();
	return pdev->readDeviceInfo();
}


MappedDevice::RegisterAccessor::RegisterAccessor(const std::string & /*_regName*/,
		const RegisterInfoMap::RegisterInfo &_me,
		ptrdev _pdev)
: me(_me),
	pdev(_pdev),
	_fixedPointConverter(_me.reg_width, _me.reg_frac_bits, _me.reg_signed) {}


void MappedDevice::RegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &me,
		size_t dataSize,
		uint32_t addRegOffset,
		uint32_t &retDataSize,
		uint32_t &retRegOff) {
	if (addRegOffset % 4) {
		throw MappedDeviceException("Register offset must be divisible by 4",
				MappedDeviceException::EX_WRONG_PARAMETER);
	}
	if (dataSize) {
		if (dataSize % 4) {
			throw MappedDeviceException("Data size must be divisible by 4",
					MappedDeviceException::EX_WRONG_PARAMETER);
		}
		if (dataSize > me.reg_size - addRegOffset) {
			throw MappedDeviceException("Data size exceed register size",
					MappedDeviceException::EX_WRONG_PARAMETER);
		}
		retDataSize = dataSize;
	} else {
		retDataSize = me.reg_size;
	}
	retRegOff = me.reg_address + addRegOffset;
}


void MappedDevice::RegisterAccessor::readRaw(int32_t *data, size_t dataSize,
		uint32_t addRegOffset) const {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	pdev->read(me.reg_bar, retRegOff, data, retDataSize);
}


void MappedDevice::RegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
		uint32_t addRegOffset) {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	pdev->write(me.reg_bar, retRegOff, data, retDataSize);
}


void MappedDevice::RegisterAccessor::readDMA(int32_t *data, size_t dataSize,
		uint32_t addRegOffset) const {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	if (me.reg_bar != 0xD) {
		throw MappedDeviceException("Cannot read data from register \"" + me.reg_name +
				"\" through DMA",
				MappedDeviceException::EX_WRONG_PARAMETER);
	}
	pdev->readDMA(me.reg_bar, retRegOff, data, retDataSize);
}


void MappedDevice::RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize,
		uint32_t addRegOffset) {
	uint32_t retDataSize;
	uint32_t retRegOff;
	checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
	if (me.reg_bar != 0xD) {
		throw MappedDeviceException("Cannot read data from register \"" + me.reg_name +
				"\" through DMA",
				MappedDeviceException::EX_WRONG_PARAMETER);
	}
	pdev->writeDMA(me.reg_bar, retRegOff, data, retDataSize);
}


RegisterInfoMap::RegisterInfo const &MappedDevice::RegisterAccessor::getRegisterInfo() const {
	return me; // me is the RegisterInfoent
}


FixedPointConverter const &MappedDevice::RegisterAccessor::getFixedPointConverter()
const {
	return _fixedPointConverter;
}



void MappedDevice::checkPointersAreNotNull() const {
	if ((pdev == false) || (registerMap == false)) {
		throw MappedDeviceException("MappedDevice has not been opened correctly",
				MappedDeviceException::EX_NOT_OPENED);
	}
}





void MappedDevice::open(boost::shared_ptr<BaseDevice> baseDevice,
		const std::string &mapFile)
{
	pdev = baseDevice;
	pdev->open();
	mapFileName = mapFile;
	registerMap = mtca4u::mapFileParser().parse(mapFileName);
}

void MappedDevice::open(std::string const & aliasName) {
	DeviceFactory FactoryInstance = DeviceFactory::getInstance();
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
