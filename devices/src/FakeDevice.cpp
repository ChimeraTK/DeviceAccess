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

FakeDevice::FakeDevice(std::string host, std::string instance, std::list<std::string> parameters)
: BaseDeviceImpl(host,instance,parameters),
	_pcieMemory(0), _pcieMemoryFileName()
{
#ifdef _DEBUG
	std::cout<<"fake is connected"<<std::endl;
#endif
}

FakeDevice::~FakeDevice()         
{
	close();
}

void FakeDevice::open()
{
#ifdef _DEBUG
	std::cout<<"open fake dev"<<std::endl;
#endif
	open(_instance);
}

void FakeDevice::open(const std::string &devName, int /*perm*/, DeviceConfigBase* /*pConfig*/)
{     
	std::string name = "./" + devName;
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

void FakeDevice::close()
{
	if (_opened == true){
		fclose(_pcieMemory);
	}
	_opened = false;
}

void FakeDevice::readInternal(uint8_t bar, uint32_t address, int32_t* data)
{   
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (bar >= MTCA4U_LIBDEV_BAR_NR) {
		throw FakeDeviceException("Wrong bar number", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	if (address >= MTCA4U_LIBDEV_BAR_MEM_SIZE) {
		throw FakeDeviceException("Wrong offset", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}

	if (fseek(_pcieMemory, address + MTCA4U_LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
		throw FakeDeviceException("Cannot access memory file", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}

	if (fread(data, sizeof(int), 1, _pcieMemory) == 0){
		throw FakeDeviceException("Cannot read memory file", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
}

void FakeDevice::writeInternal(uint8_t bar, uint32_t address, int32_t data)
{    
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (bar >= MTCA4U_LIBDEV_BAR_NR) {
		throw FakeDeviceException("Wrong bar number", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	if (address >= MTCA4U_LIBDEV_BAR_MEM_SIZE) {
		throw FakeDeviceException("Wrong offset", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	if (fseek(_pcieMemory, address + MTCA4U_LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
		throw FakeDeviceException("Cannot access memory file", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	if (fwrite(&data, sizeof(int), 1, _pcieMemory) == 0){
		throw FakeDeviceException("Cannot write memory file", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
}

void FakeDevice::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes){
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}

	if ( sizeInBytes % 4 ) {
		throw FakeDeviceException("Wrong data size - must be dividable by 4", FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	for (uint16_t i = 0; i < sizeInBytes/4; i++){
		readInternal(bar, address + i*4, data + i);
	}
}

void FakeDevice::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes)
{       
	if (_opened == false) {
		throw FakeDeviceException("Device closed", FakeDeviceException::EX_DEVICE_CLOSED);
	}
	if (sizeInBytes % 4) {
		throw FakeDeviceException("Wrong data size - must be divisible by 4", FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	for (uint16_t i = 0; i < sizeInBytes/4; i++){
		writeInternal(bar, address + i*4, *(data + i));
	}
}

void FakeDevice::readDMA(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes)
{   
	read(bar, address, data, sizeInBytes);
}

void FakeDevice::writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes)
{
	write(bar, address, data, sizeInBytes);
}


std::string FakeDevice::readDeviceInfo()
{
	return std::string("fake device: ") + _pcieMemoryFileName;
}

boost::shared_ptr<BaseDevice> FakeDevice::createInstance(std::string host, std::string instance, std::list<std::string> parameters) {
	return boost::shared_ptr<BaseDevice> ( new FakeDevice(host,instance,parameters));
}

}//namespace mtca4u
