#include "FakeDevice.h"
#include "FakeDeviceException.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <cstdio>
namespace mtca4u{

FakeDevice::FakeDevice()
: _pcieMemory(0), _pcieMemoryFileName()
{

}

FakeDevice::FakeDevice(std::string host, std::string interface, std::list<std::string> parameters)
: BaseDeviceImpl(host,interface,parameters),
	_pcieMemory(0), _pcieMemoryFileName()
{
#ifdef _DEBUG
	std::cout<<"fake is connected"<<std::endl;
#endif
}

FakeDevice::~FakeDevice()         
{
	closeDev();
}

void FakeDevice::openDev()
{
#ifdef _DEBUG
	std::cout<<"open fake dev"<<std::endl;
#endif
	openDev(_interface);
}

void FakeDevice::openDev(const std::string &devName, int /*perm*/, DeviceConfigBase* /*pConfig*/)
{     
	std::string name = "./" + devName;
	std::cout<<"name:"<<name<<std::endl;
	std::replace(name.begin(), name.end(), '/', '_');
	_pcieMemoryFileName = name;
	if (_opened == true) {
		throw FakeDeviceException("Device already has been opened", FakeDeviceException::EX_DEVICE_OPENED);
	}
	_pcieMemory = fopen(name.c_str(), "r+");
	if (_pcieMemory == NULL){
		char zero[MTCA4U_LIBDEV_BAR_MEM_SIZE] = {0};
		_pcieMemory = fopen(name.c_str(), "w");
		if (_pcieMemory == NULL){
			throw FakeDeviceException("Cannot create fake device file", FakeDeviceException::EX_CANNOT_CREATE_DEV_FILE);
		}
		for (int bar = 0; bar < MTCA4U_LIBDEV_BAR_NR; bar++){
			if (fseek(_pcieMemory, sizeof(zero) * bar, SEEK_SET) < 0){
				fclose(_pcieMemory);
				throw FakeDeviceException("Cannot init device memory file", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
			}
			if (fwrite(zero, sizeof(zero), 1, _pcieMemory) == 0){
				fclose(_pcieMemory);
				throw FakeDeviceException("Cannot init device memory file", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
			}
		}
		fclose(_pcieMemory);
		_pcieMemory = fopen(name.c_str(), "r+");
	}
	_opened = true;
}

void FakeDevice::closeDev()
{
	if (_opened == true){
		fclose(_pcieMemory);
	}
	_opened = false;
}

void FakeDevice::readReg(uint32_t regOffset, int32_t* data, uint8_t bar)
{   
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (bar >= MTCA4U_LIBDEV_BAR_NR) {
		throw FakeDeviceException("Wrong bar number", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	if (regOffset >= MTCA4U_LIBDEV_BAR_MEM_SIZE) {
		throw FakeDeviceException("Wrong offset", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}

	if (fseek(_pcieMemory, regOffset + MTCA4U_LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
		throw FakeDeviceException("Cannot access memory file", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}

	if (fread(data, sizeof(int), 1, _pcieMemory) == 0){
		throw FakeDeviceException("Cannot read memory file", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
}

void FakeDevice::writeReg(uint32_t regOffset, int32_t data, uint8_t bar)
{    
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (bar >= MTCA4U_LIBDEV_BAR_NR) {
		throw FakeDeviceException("Wrong bar number", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	if (regOffset >= MTCA4U_LIBDEV_BAR_MEM_SIZE) {
		throw FakeDeviceException("Wrong offset", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	if (fseek(_pcieMemory, regOffset + MTCA4U_LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
		throw FakeDeviceException("Cannot access memory file", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	if (fwrite(&data, sizeof(int), 1, _pcieMemory) == 0){
		throw FakeDeviceException("Cannot write memory file", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
}

void FakeDevice::readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{       
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (size % 4) {
		throw FakeDeviceException("Wrong data size - must be dividable by 4", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	for (uint16_t i = 0; i < size/4; i++){
		readReg(regOffset + i*4, data + i, bar);
	}
}

void FakeDevice::writeArea(uint32_t regOffset, int32_t const * data, size_t size, uint8_t bar)
{       
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (size % 4) {
		throw FakeDeviceException("Wrong data size - must be divisible by 4", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	for (uint16_t i = 0; i < size/4; i++){
		writeReg(regOffset + i*4, *(data + i), bar);
	}
}

void FakeDevice::readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{   
	readArea(regOffset, data, size, bar);
}

void FakeDevice::writeDMA(uint32_t regOffset, int32_t const * data, size_t size, uint8_t bar)
{
	writeArea(regOffset, data, size, bar);
}

void FakeDevice::readDeviceInfo(std::string* devInfo)
{
	*devInfo = "fake device: " + _pcieMemoryFileName;
}

BaseDevice* FakeDevice::createInstance(std::string host, std::string interface, std::list<std::string> parameters) {
	return new FakeDevice(host,interface,parameters);
}

}//namespace mtca4u
