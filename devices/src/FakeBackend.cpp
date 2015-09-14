#include "FakeBackend.h"
#include "FakeBackendException.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <cstdio>
namespace mtca4u{

FakeBackend::FakeBackend()
: _pcieMemory(0), _pcieMemoryFileName()
{

}

FakeBackend::FakeBackend(std::string host, std::string instance, std::list<std::string> parameters)
: DeviceBackendImpl(host,instance,parameters),
	_pcieMemory(0), _pcieMemoryFileName()
{
#ifdef _DEBUG
	std::cout<<"fake is connected"<<std::endl;
#endif
}

FakeBackend::~FakeBackend()         
{
	close();
}

void FakeBackend::open()
{
#ifdef _DEBUG
	std::cout<<"open fake dev"<<std::endl;
#endif
	open(_instance);
}

void FakeBackend::open(const std::string &devName, int /*perm*/, DeviceConfigBase* /*pConfig*/)
{     
	std::string name = "./" + devName;
	std::replace(name.begin(), name.end(), '/', '_');
	_pcieMemoryFileName = name;
	if (_opened == true) {
		throw FakeBackendException("Device already has been opened", FakeBackendException::EX_DEVICE_OPENED);
	}
	_pcieMemory = fopen(name.c_str(), "r+");
	if (_pcieMemory == NULL){
		char zero[MTCA4U_LIBDEV_BAR_MEM_SIZE] = {0};
		_pcieMemory = fopen(name.c_str(), "w");
		if (_pcieMemory == NULL){
			throw FakeBackendException("Cannot create fake device file", FakeBackendException::EX_CANNOT_CREATE_DEV_FILE);
		}
		for (int bar = 0; bar < MTCA4U_LIBDEV_BAR_NR; bar++){
			if (fseek(_pcieMemory, sizeof(zero) * bar, SEEK_SET) < 0){
				fclose(_pcieMemory);
				throw FakeBackendException("Cannot init device memory file", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
			}
			if (fwrite(zero, sizeof(zero), 1, _pcieMemory) == 0){
				fclose(_pcieMemory);
				throw FakeBackendException("Cannot init device memory file", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
			}
		}
		fclose(_pcieMemory);
		_pcieMemory = fopen(name.c_str(), "r+");
	}
	_opened = true;
}

void FakeBackend::close()
{
	if (_opened == true){
		fclose(_pcieMemory);
	}
	_opened = false;
}

void FakeBackend::readInternal(uint8_t bar, uint32_t address, int32_t* data)
{   
	if (_opened == false) {
		throw FakeBackendException("Device closed", FakeBackendException::EX_DEVICE_CLOSED);
	}
	if (bar >= MTCA4U_LIBDEV_BAR_NR) {
		throw FakeBackendException("Wrong bar number", FakeBackendException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	if (address >= MTCA4U_LIBDEV_BAR_MEM_SIZE) {
		throw FakeBackendException("Wrong offset", FakeBackendException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}

	if (fseek(_pcieMemory, address + MTCA4U_LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
		throw FakeBackendException("Cannot access memory file", FakeBackendException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}

	if (fread(data, sizeof(int), 1, _pcieMemory) == 0){
		throw FakeBackendException("Cannot read memory file", FakeBackendException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
}

void FakeBackend::writeInternal(uint8_t bar, uint32_t address, int32_t data)
{    
	if (_opened == false) {
		throw FakeBackendException("Device closed", FakeBackendException::EX_DEVICE_CLOSED);
	}
	if (bar >= MTCA4U_LIBDEV_BAR_NR) {
		throw FakeBackendException("Wrong bar number", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	if (address >= MTCA4U_LIBDEV_BAR_MEM_SIZE) {
		throw FakeBackendException("Wrong offset", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	if (fseek(_pcieMemory, address + MTCA4U_LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
		throw FakeBackendException("Cannot access memory file", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	if (fwrite(&data, sizeof(int), 1, _pcieMemory) == 0){
		throw FakeBackendException("Cannot write memory file", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
}

void FakeBackend::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes){
	if (_opened == false) {
		throw FakeBackendException("Device closed", FakeBackendException::EX_DEVICE_CLOSED);
	}

	if ( sizeInBytes % 4 ) {
		throw FakeBackendException("Wrong data size - must be dividable by 4", FakeBackendException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	for (uint16_t i = 0; i < sizeInBytes/4; i++){
		readInternal(bar, address + i*4, data + i);
	}
}

void FakeBackend::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes)
{       
	if (_opened == false) {
		throw FakeBackendException("Device closed", FakeBackendException::EX_DEVICE_CLOSED);
	}
	if (sizeInBytes % 4) {
		throw FakeBackendException("Wrong data size - must be divisible by 4", FakeBackendException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	for (uint16_t i = 0; i < sizeInBytes/4; i++){
		writeInternal(bar, address + i*4, *(data + i));
	}
}

void FakeBackend::readDMA(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes)
{   
	read(bar, address, data, sizeInBytes);
}

void FakeBackend::writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes)
{
	write(bar, address, data, sizeInBytes);
}


std::string FakeBackend::readDeviceInfo()
{
	return std::string("fake device: ") + _pcieMemoryFileName;
}

boost::shared_ptr<DeviceBackend> FakeBackend::createInstance(std::string host, std::string instance, std::list<std::string> parameters) {
	return boost::shared_ptr<DeviceBackend> ( new FakeBackend(host,instance,parameters));
}

}//namespace mtca4u
