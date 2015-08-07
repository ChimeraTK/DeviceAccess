#include <iostream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sstream>
#include <unistd.h>

#include <boost/bind.hpp>
//#include <boost/lambda.hpp>

// the io constants and struct for the driver
// FIXME: they should come from the installed driver
#include <pciedev_io.h>
#include <pcieuni_io_compat.h>
#include <llrfdrv_io_compat.h>
#include "PcieDevice.h"
#include "ExcPcieDevice.h"
namespace mtca4u {

PcieDevice::PcieDevice()
	:_deviceID(0),
	_ioctlPhysicalSlot(0),
	_ioctlDriverVersion(0) {}

PcieDevice::PcieDevice(std::string host, std::string interface, std::list<std::string> parameters)
: BaseDeviceImpl(host,interface,parameters)
, _deviceID(0),
_ioctlPhysicalSlot(0),
_ioctlDriverVersion(0)
{
	//temp
	_interface = "/dev/"+_interface;
#ifdef _DEBUG
	std::cout<<"pci is connected"<<std::endl;
#endif
}

PcieDevice::~PcieDevice() { closeDev(); }

void PcieDevice::openDev() {
#ifdef _DEBUG
	std::cout << "open pcie dev" << std::endl;
#endif
	openDev(_interface);
}

void PcieDevice::openDev(const std::string& devName, int perm,
		DeviceConfigBase* /*pConfig*/) {
	if (_opened == true) {
		throw ExcPcieDevice("Device already has been _Opened",
				ExcPcieDevice::EX_DEVICE_OPENED);
	}
	_interface =  devName; //Todo cleanup
	_deviceID = open(devName.c_str(), perm);
	if (_deviceID < 0) {
		throw ExcPcieDevice(createErrorStringWithErrnoText("Cannot open device: "),
				ExcPcieDevice::EX_CANNOT_OPEN_DEVICE);
	}

	determineDriverAndConfigureIoctl();

	_opened = true;
}

void PcieDevice::determineDriverAndConfigureIoctl() {
	// determine the driver by trying the physical slot ioctl
	device_ioctrl_data ioctlData = { 0, 0, 0, 0 };

	if (ioctl(_deviceID, PCIEDEV_PHYSICAL_SLOT, &ioctlData) >= 0) {
		// it's the pciedev driver
		_ioctlPhysicalSlot = PCIEDEV_PHYSICAL_SLOT;
		_ioctlDriverVersion = PCIEDEV_DRIVER_VERSION;
		_ioctlDMA = PCIEDEV_READ_DMA;
		_readDMAFunction =
				boost::bind(&PcieDevice::readDMAViaIoctl, this, _1, _2, _3, _4);
		_writeFunction =
				boost::bind(&PcieDevice::writeWithStruct, this, _1, _2, _3);
		_writeAreaFunction =
				boost::bind(&PcieDevice::writeAreaWithStruct, this, _1, _2, _3, _4);
		_readFunction = boost::bind(&PcieDevice::readWithStruct, this, _1, _2, _3);
		_readAreaFunction =
				boost::bind(&PcieDevice::readAreaWithStruct, this, _1, _2, _3, _4);
		return;
	}

	if (ioctl(_deviceID, LLRFDRV_PHYSICAL_SLOT, &ioctlData) >= 0) {
		// it's the llrf driver
		_ioctlPhysicalSlot = LLRFDRV_PHYSICAL_SLOT;
		_ioctlDriverVersion = LLRFDRV_DRIVER_VERSION;
		_ioctlDMA = 0;
		_readDMAFunction =
				boost::bind(&PcieDevice::readDMAViaStruct, this, _1, _2, _3, _4);
		_writeFunction =
				boost::bind(&PcieDevice::writeWithStruct, this, _1, _2, _3);
		_writeAreaFunction =
				boost::bind(&PcieDevice::writeAreaWithStruct, this, _1, _2, _3, _4);
		_readFunction = boost::bind(&PcieDevice::readWithStruct, this, _1, _2, _3);
		_readAreaFunction =
				boost::bind(&PcieDevice::readAreaWithStruct, this, _1, _2, _3, _4);
		return;
	}

	if (ioctl(_deviceID, PCIEUNI_PHYSICAL_SLOT, &ioctlData) >= 0) {
		// it's the pcieuni
		_ioctlPhysicalSlot = PCIEUNI_PHYSICAL_SLOT;
		_ioctlDriverVersion = PCIEUNI_DRIVER_VERSION;
		_ioctlDMA = PCIEUNI_READ_DMA;
		_readDMAFunction =
				boost::bind(&PcieDevice::readDMAViaIoctl, this, _1, _2, _3, _4);
		_writeFunction = boost::bind(&PcieDevice::directWrite, this, _1, _2, _3,
				sizeof(int32_t));
		_writeAreaFunction =
				boost::bind(&PcieDevice::directWrite, this, _1, _2, _3, _4);
		_readFunction =
				boost::bind(&PcieDevice::directRead, this, _1, _2, _3, sizeof(int32_t));
		_readAreaFunction =
				boost::bind(&PcieDevice::directRead, this, _1, _2, _3, _4);
		return;
	}

	// No working driver. Close the device and throw an exception.
	std::cerr << "Unsupported driver. "
			<< createErrorStringWithErrnoText("Error is ") << std::endl;
	;
	close(_deviceID);
	throw ExcPcieDevice("Unsupported driver in device" + _interface,
			ExcPcieDevice::EX_UNSUPPORTED_DRIVER);
}

void PcieDevice::closeDev() {
	if (_opened == true) {
		close(_deviceID);
	}
	_opened = false;
}

void PcieDevice::readWithStruct(uint32_t regOffset, int32_t* data,
		uint8_t bar) {
	device_rw l_RW;
	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}
	l_RW.barx_rw = bar;
	l_RW.mode_rw = RW_D32;
	l_RW.offset_rw = regOffset;
	l_RW.size_rw =
			0; // does not overwrite the struct but writes one word back to data
	l_RW.data_rw = -1;
	l_RW.rsrvd_rw = 0;

	if (read(_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)) {
		throw ExcPcieDevice(
				createErrorStringWithErrnoText("Cannot read data from device: "),
				ExcPcieDevice::EX_READ_ERROR);
	}
	*data = l_RW.data_rw;
}

void PcieDevice::directRead(uint32_t regOffset, int32_t* data, uint8_t bar,
		size_t sizeInBytes) {
	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}
	if (bar > 5) {
		std::stringstream errorMessage;
		errorMessage << "Invalid bar number: " << bar << std::endl;
		throw ExcPcieDevice(errorMessage.str(), ExcPcieDevice::EX_READ_ERROR);
	}
	loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + regOffset;

	if (pread(_deviceID, data, sizeInBytes, virtualOffset) !=
			static_cast<int>(sizeInBytes)) {
		throw ExcPcieDevice(
				createErrorStringWithErrnoText("Cannot read data from device: "),
				ExcPcieDevice::EX_READ_ERROR);
	}
}

void PcieDevice::readReg(uint32_t regOffset, int32_t* data, uint8_t bar) {
	_readFunction(regOffset, data, bar);
}

void PcieDevice::writeWithStruct(uint32_t regOffset, int32_t const* data,
		uint8_t bar) {
	device_rw l_RW;
	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}
	l_RW.barx_rw = bar;
	l_RW.mode_rw = RW_D32;
	l_RW.offset_rw = regOffset;
	l_RW.data_rw = *data;
	l_RW.rsrvd_rw = 0;
	l_RW.size_rw = 0;

	if (write(_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)) {
		throw ExcPcieDevice(
				createErrorStringWithErrnoText("Cannot write data to device: "),
				ExcPcieDevice::EX_WRITE_ERROR);
	}
}

// direct write allows to read areas directly, without a loop in user space
void PcieDevice::directWrite(uint32_t regOffset, int32_t const* data,
		uint8_t bar, size_t sizeInBytes) {
	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}
	if (bar > 5) {
		std::stringstream errorMessage;
		errorMessage << "Invalid bar number: " << bar << std::endl;
		throw ExcPcieDevice(errorMessage.str(), ExcPcieDevice::EX_WRITE_ERROR);
	}
	loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + regOffset;

	if (pwrite(_deviceID, data, sizeInBytes, virtualOffset) !=
			static_cast<int>(sizeInBytes)) {
		throw ExcPcieDevice(
				createErrorStringWithErrnoText("Cannot write data to device: "),
				ExcPcieDevice::EX_WRITE_ERROR);
	}
}

void PcieDevice::writeReg(uint32_t regOffset, int32_t data, uint8_t bar) {
	_writeFunction(regOffset, &data, bar);
}

void PcieDevice::readAreaWithStruct(uint32_t regOffset, int32_t* data,
		uint8_t bar, size_t size) {
	if (size % 4) {
		throw ExcPcieDevice("Wrong data size - must be dividable by 4",
				ExcPcieDevice::EX_READ_ERROR);
	}

	for (uint32_t i = 0; i < size / 4; i++) {
		readReg(regOffset + i * 4, data + i, bar);
	}
}

void PcieDevice::readArea(uint32_t regOffset, int32_t* data, size_t size,
		uint8_t bar) {
	// Yes I know, the order of bar and size is reversed. The writeArea interface
	// got it wrong and I wanted to break it to keep the internal functions nice.
	_readAreaFunction(regOffset, data, bar, size);
}

void PcieDevice::writeAreaWithStruct(uint32_t regOffset, int32_t const* data,
		uint8_t bar, size_t size) {
	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}
	if (size % 4) {
		throw ExcPcieDevice("Wrong data size - must be dividable by 4",
				ExcPcieDevice::EX_WRITE_ERROR);
	}
	for (uint32_t i = 0; i < size / 4; i++) {
		writeReg(regOffset + i * 4, *(data + i), bar);
	}
}

void PcieDevice::writeArea(uint32_t regOffset, int32_t const* data, size_t size,
		uint8_t bar) {
	// Yes I know, the order of bar and size is reversed. The writeArea interface
	// got it wrong and I wanted to break it to keep the internal functions nice.
	_writeAreaFunction(regOffset, data, bar, size);
}

void PcieDevice::readDMA(uint32_t regOffset, int32_t* data, size_t size,
		uint8_t bar) {
	_readDMAFunction(regOffset, data, size, bar);
}

void PcieDevice::readDMAViaStruct(uint32_t regOffset, int32_t* data,
		size_t size, uint8_t /*bar*/) {
	ssize_t ret;
	device_rw l_RW;
	device_rw* pl_RW;

	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}
	if (size < sizeof(device_rw)) {
		pl_RW = &l_RW;
	} else {
		pl_RW = (device_rw*)data;
	}

	pl_RW->data_rw = 0;
	pl_RW->barx_rw = 0;
	pl_RW->size_rw = size;
	pl_RW->mode_rw = RW_DMA;
	pl_RW->offset_rw = regOffset;
	pl_RW->rsrvd_rw = 0;

	ret = read(_deviceID, pl_RW, sizeof(device_rw));
	if (ret != (ssize_t)size) {
		throw ExcPcieDevice(
				createErrorStringWithErrnoText("Cannot read data from device: "),
				ExcPcieDevice::EX_DMA_READ_ERROR);
	}
	if (size < sizeof(device_rw)) {
		memcpy(data, pl_RW, size);
	}
}

void PcieDevice::readDMAViaIoctl(uint32_t regOffset, int32_t* data, size_t size,
		uint8_t /*bar*/) {
	if (_opened == false) {
		throw ExcPcieDevice("Device closed", ExcPcieDevice::EX_DEVICE_CLOSED);
	}

	// safety check: the requested dma size (size of the data buffer) has to be at
	// least
	// the size of the dma struct, because the latter has to be copied into the
	// data buffer.
	if (size < sizeof(device_ioctrl_dma)) {
		throw ExcPcieDevice("Reqested dma size is too small",
				ExcPcieDevice::EX_DMA_READ_ERROR);
	}

	// prepare the struct
	device_ioctrl_dma DMA_RW;
	DMA_RW.dma_cmd = 0;     // FIXME: Why is it 0? => read driver code
	DMA_RW.dma_pattern = 0; // FIXME: Why is it 0? => read driver code
	DMA_RW.dma_size = size;
	DMA_RW.dma_offset = regOffset;
	DMA_RW.dma_reserved1 = 0; // FIXME: is this a correct value?
	DMA_RW.dma_reserved2 = 0; // FIXME: is this a correct value?

	// the ioctrl_dma struct is copied to the beginning of the data buffer,
	// so the information about size and offset are passed to the driver.
	memcpy((void*)data, &DMA_RW, sizeof(device_ioctrl_dma));
	int ret = ioctl(_deviceID, _ioctlDMA, (void*)data);
	if (ret) {
		throw ExcPcieDevice(
				createErrorStringWithErrnoText("Cannot read data from device "),
				ExcPcieDevice::EX_DMA_READ_ERROR);
	}
}

void PcieDevice::writeDMA(uint32_t /*regOffset*/, int32_t const* /*data*/,
		size_t /*size*/, uint8_t /*bar*/) {
	throw ExcPcieDevice("Operation not supported yet", ExcPcieDevice::EX_DMA_WRITE_ERROR);
}

void PcieDevice::readDeviceInfo(std::string* devInfo) {
	std::ostringstream os;
	device_ioctrl_data ioctlData = { 0, 0, 0, 0 };
	if (ioctl(_deviceID, _ioctlPhysicalSlot, &ioctlData) < 0) {
		throw ExcPcieDevice(createErrorStringWithErrnoText("Cannot read device info: "),
				ExcPcieDevice::EX_INFO_READ_ERROR);
	}
	os << "SLOT: " << ioctlData.data;
	if (ioctl(_deviceID, _ioctlDriverVersion, &ioctlData) < 0) {
		throw ExcPcieDevice(createErrorStringWithErrnoText("Cannot read device info: "),
				ExcPcieDevice::EX_INFO_READ_ERROR);
	}
	os << " DRV VER: " << (float)(ioctlData.offset / 10.0) +
			(float)ioctlData.data;
	*devInfo = os.str();
}

std::string PcieDevice::createErrorStringWithErrnoText(
		std::string const& startText) {
	char errorBuffer[255];
	return startText + _interface + ": " +
			strerror_r(errno, errorBuffer, sizeof(errorBuffer));
}


BaseDevice* PcieDevice::createInstance(std::string host, std::string interface, std::list<std::string> parameters) {
	return new PcieDevice(host,interface,parameters);
}

} // namespace mtca4u
