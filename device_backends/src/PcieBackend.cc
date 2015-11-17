#include <iostream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sstream>
#include <unistd.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

// the io constants and struct for the driver
// FIXME: they should come from the installed driver
#include <pciedev_io.h>
#include <pcieuni_io_compat.h>
#include <llrfdrv_io_compat.h>
#include "PcieBackend.h"
#include "PcieBackendException.h"
namespace mtca4u {

  PcieBackend::PcieBackend(std::string deviceNodeName)
  : _deviceID(0),
    _ioctlPhysicalSlot(0),
    _ioctlDriverVersion(0),
    _ioctlDMA(0),
    _deviceNodeName(deviceNodeName){
  }

  PcieBackend::~PcieBackend(){
    close();
  }

  void PcieBackend::open() {
#ifdef _DEBUG
    std::cout << "open pcie dev" << std::endl;
#endif
    if (_opened) {
      throw PcieBackendException("Device already has been _Opened",
          PcieBackendException::EX_DEVICE_OPENED);
    }
    _deviceID = ::open(_deviceNodeName.c_str(), O_RDWR);
    if (_deviceID < 0) {
      throw PcieBackendException(createErrorStringWithErrnoText("Cannot open device: "),
          PcieBackendException::EX_CANNOT_OPEN_DEVICE);
    }

    determineDriverAndConfigureIoctl();

    _opened = true;
  }

  void PcieBackend::determineDriverAndConfigureIoctl() {
    // determine the driver by trying the physical slot ioctl
    device_ioctrl_data ioctlData = { 0, 0, 0, 0 };

    if (ioctl(_deviceID, PCIEDEV_PHYSICAL_SLOT, &ioctlData) >= 0) {
      // it's the pciedev driver
      _ioctlPhysicalSlot = PCIEDEV_PHYSICAL_SLOT;
      _ioctlDriverVersion = PCIEDEV_DRIVER_VERSION;
      _ioctlDMA = PCIEDEV_READ_DMA;
      _readDMAFunction =
          boost::bind(&PcieBackend::readDMAViaIoctl, this, _1, _2, _3, _4);
      _writeFunction =
          boost::bind(&PcieBackend::writeWithStruct, this, _1, _2, _3, _4);
      _readFunction =
          boost::bind(&PcieBackend::readWithStruct, this, _1, _2, _3, _4);
      return;
    }

    if (ioctl(_deviceID, LLRFDRV_PHYSICAL_SLOT, &ioctlData) >= 0) {
      // it's the llrf driver
      _ioctlPhysicalSlot = LLRFDRV_PHYSICAL_SLOT;
      _ioctlDriverVersion = LLRFDRV_DRIVER_VERSION;
      _ioctlDMA = 0;
      _readDMAFunction =
          boost::bind(&PcieBackend::readDMAViaStruct, this, _1, _2, _3, _4);
      _writeFunction =
          boost::bind(&PcieBackend::writeWithStruct, this, _1, _2, _3, _4);
      _readFunction =
          boost::bind(&PcieBackend::readWithStruct, this, _1, _2, _3, _4);
      return;
    }

    if (ioctl(_deviceID, PCIEUNI_PHYSICAL_SLOT, &ioctlData) >= 0) {
      // it's the pcieuni
      _ioctlPhysicalSlot = PCIEUNI_PHYSICAL_SLOT;
      _ioctlDriverVersion = PCIEUNI_DRIVER_VERSION;
      _ioctlDMA = PCIEUNI_READ_DMA;
      _readDMAFunction =
          boost::bind(&PcieBackend::readDMAViaIoctl, this, _1, _2, _3, _4);
      _writeFunction =
          boost::bind(&PcieBackend::directWrite, this, _1, _2, _3, _4);
      _readFunction =
          boost::bind(&PcieBackend::directRead, this, _1, _2, _3, sizeof(int32_t));
      _readFunction =
          boost::bind(&PcieBackend::directRead, this, _1, _2, _3, _4);
      return;
    }

    // No working driver. Close the device and throw an exception.
    std::cerr << "Unsupported driver. "
        << createErrorStringWithErrnoText("Error is ") << std::endl;
    ::close(_deviceID);
    throw PcieBackendException("Unsupported driver in device" + _deviceNodeName,
        PcieBackendException::EX_UNSUPPORTED_DRIVER);
  }

  void PcieBackend::close() {
    if (_opened) {
      ::close(_deviceID);
    }
    _opened = false;
  }

  void PcieBackend::readInternal(uint8_t bar, uint32_t address, int32_t* data) {
    device_rw l_RW;
    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }
    l_RW.barx_rw = bar;
    l_RW.mode_rw = RW_D32;
    l_RW.offset_rw = address;
    l_RW.size_rw =
        0; // does not overwrite the struct but writes one word back to data
    l_RW.data_rw = -1;
    l_RW.rsrvd_rw = 0;

    if (::read(_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)) {
      throw PcieBackendException(
          createErrorStringWithErrnoText("Cannot read data from device: "),
          PcieBackendException::EX_READ_ERROR);
    }
    *data = l_RW.data_rw;
  }

  void PcieBackend::directRead(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }
    if (bar > 5) {
      std::stringstream errorMessage;
      errorMessage << "Invalid bar number: " << bar << std::endl;
      throw PcieBackendException(errorMessage.str(), PcieBackendException::EX_READ_ERROR);
    }
    loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + address;

    if (pread(_deviceID, data, sizeInBytes, virtualOffset) !=
        static_cast<int>(sizeInBytes)) {
      throw PcieBackendException(
          createErrorStringWithErrnoText("Cannot read data from device: "),
          PcieBackendException::EX_READ_ERROR);
    }
  }

  void PcieBackend::writeInternal(uint8_t bar, uint32_t address, int32_t const* data) {
    device_rw l_RW;
    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }
    l_RW.barx_rw = bar;
    l_RW.mode_rw = RW_D32;
    l_RW.offset_rw = address;
    l_RW.data_rw = *data;
    l_RW.rsrvd_rw = 0;
    l_RW.size_rw = 0;

    if (::write(_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)) {
      throw PcieBackendException(
          createErrorStringWithErrnoText("Cannot write data to device: "),
          PcieBackendException::EX_WRITE_ERROR);
    }
  }

  // direct write allows to read areas directly, without a loop in user space
  void PcieBackend::directWrite(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }
    if (bar > 5) {
      std::stringstream errorMessage;
      errorMessage << "Invalid bar number: " << bar << std::endl;
      throw PcieBackendException(errorMessage.str(), PcieBackendException::EX_WRITE_ERROR);
    }
    loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + address;

    if (pwrite(_deviceID, data, sizeInBytes, virtualOffset) !=
        static_cast<int>(sizeInBytes)) {
      throw PcieBackendException(
          createErrorStringWithErrnoText("Cannot write data to device: "),
          PcieBackendException::EX_WRITE_ERROR);
    }
  }

  void PcieBackend::readWithStruct(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
    if (sizeInBytes % 4) {
      throw PcieBackendException("Wrong data size - must be dividable by 4",
          PcieBackendException::EX_READ_ERROR);
    }

    for (uint32_t i = 0; i < sizeInBytes / 4; i++) {
      readInternal(bar, address + i * 4, data + i);
    }
  }

  void PcieBackend::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes)
  {
    if(bar != 0xD) {
      _readFunction(bar, address, data, sizeInBytes);
    }
    else {
      _readDMAFunction(bar, address, data, sizeInBytes );
    }
  }

  void PcieBackend::writeWithStruct(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }
    if (sizeInBytes % 4) {
      throw PcieBackendException("Wrong data size - must be dividable by 4",
          PcieBackendException::EX_WRITE_ERROR);
    }
    for (uint32_t i = 0; i < sizeInBytes / 4; i++) {
      writeInternal(bar, address + i * 4, (data + i));
    }
  }

  void PcieBackend::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
    _writeFunction(bar, address, data, sizeInBytes);
  }

  void PcieBackend::readDMAViaStruct(uint8_t /*bar*/, uint32_t address, int32_t* data,  size_t sizeInBytes) {
    ssize_t ret;
    device_rw l_RW;
    device_rw* pl_RW;

    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }
    if (sizeInBytes < sizeof(device_rw)) {
      pl_RW = &l_RW;
    } else {
      pl_RW = (device_rw*)data;
    }

    pl_RW->data_rw = 0;
    pl_RW->barx_rw = 0;
    pl_RW->size_rw = sizeInBytes;
    pl_RW->mode_rw = RW_DMA;
    pl_RW->offset_rw = address;
    pl_RW->rsrvd_rw = 0;

    ret = ::read(_deviceID, pl_RW, sizeof(device_rw));
    if (ret != (ssize_t)sizeInBytes) {
      throw PcieBackendException(
          createErrorStringWithErrnoText("Cannot read data from device: "),
          PcieBackendException::EX_DMA_READ_ERROR);
    }
    if (sizeInBytes < sizeof(device_rw)) {
      memcpy(data, pl_RW, sizeInBytes);
    }
  }

  void PcieBackend::readDMAViaIoctl(uint8_t /*bar*/, uint32_t address, int32_t* data,  size_t sizeInBytes) {
    if (_opened == false) {
      throw PcieBackendException("Device closed", PcieBackendException::EX_DEVICE_CLOSED);
    }

    // safety check: the requested dma size (size of the data buffer) has to be at
    // least
    // the size of the dma struct, because the latter has to be copied into the
    // data buffer.
    if (sizeInBytes < sizeof(device_ioctrl_dma)) {
      throw PcieBackendException("Reqested dma size is too small",
          PcieBackendException::EX_DMA_READ_ERROR);
    }

    // prepare the struct
    device_ioctrl_dma DMA_RW;
    DMA_RW.dma_cmd = 0;     // FIXME: Why is it 0? => read driver code
    DMA_RW.dma_pattern = 0; // FIXME: Why is it 0? => read driver code
    DMA_RW.dma_size = sizeInBytes;
    DMA_RW.dma_offset = address;
    DMA_RW.dma_reserved1 = 0; // FIXME: is this a correct value?
    DMA_RW.dma_reserved2 = 0; // FIXME: is this a correct value?

    // the ioctrl_dma struct is copied to the beginning of the data buffer,
    // so the information about size and offset are passed to the driver.
    memcpy((void*)data, &DMA_RW, sizeof(device_ioctrl_dma));
    int ret = ioctl(_deviceID, _ioctlDMA, (void*)data);
    if (ret) {
      throw PcieBackendException(
          createErrorStringWithErrnoText("Cannot read data from device "),
          PcieBackendException::EX_DMA_READ_ERROR);
    }
  }

  std::string PcieBackend::readDeviceInfo() {
    std::ostringstream os;
    device_ioctrl_data ioctlData = { 0, 0, 0, 0 };
    if (ioctl(_deviceID, _ioctlPhysicalSlot, &ioctlData) < 0) {
      throw PcieBackendException(createErrorStringWithErrnoText("Cannot read device info: "),
          PcieBackendException::EX_INFO_READ_ERROR);
    }
    os << "SLOT: " << ioctlData.data;
    if (ioctl(_deviceID, _ioctlDriverVersion, &ioctlData) < 0) {
      throw PcieBackendException(createErrorStringWithErrnoText("Cannot read device info: "),
          PcieBackendException::EX_INFO_READ_ERROR);
    }
    os << " DRV VER: " << (float)(ioctlData.offset / 10.0) +
        (float)ioctlData.data;
    return os.str();
  }

  std::string PcieBackend::createErrorStringWithErrnoText(std::string const& startText) {
    char errorBuffer[255];
    return startText + _deviceNodeName + ": " +
        strerror_r(errno, errorBuffer, sizeof(errorBuffer));
  }

  boost::shared_ptr<DeviceBackend> PcieBackend::createInstance(std::string /*host*/,
      std::string instance,
      std::list<std::string> /*parameters*/){
    return boost::shared_ptr<DeviceBackend> (new PcieBackend("/dev/"+instance));
  }

} // namespace mtca4u
