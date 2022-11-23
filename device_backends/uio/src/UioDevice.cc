// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioDevice.h"

#include "Exception.h"
#include <sys/mman.h>

#include <fcntl.h>

namespace ChimeraTK {

  UioDevice::UioDevice(std::string deviceFilePath) : _deviceFilePath(deviceFilePath.c_str()) {
    std::string fileName = _deviceFilePath.filename().string();
    _deviceKernelBase = (void*)readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/addr");
    _deviceMemSize = readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/size");
  }

  UioDevice::~UioDevice() {}

  void UioDevice::open() {
    _deviceFileDescriptor = ::open(_deviceFilePath.c_str(), O_RDWR);
    if(_deviceFileDescriptor < 0) {
      throw ChimeraTK::runtime_error("Failed to open UIO device '" + getDeviceFilePath() + "'");
    }

    UioMMap();
  }

  void UioDevice::close() {
    UioUnmap();
    ::close(_deviceFileDescriptor);
  }

  void UioDevice::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(bar > 0) {
      throw ChimeraTK::logic_error("Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map file are addressed using absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("Read request exceeds device memory region");
    }
    void* targetAddress = static_cast<uint8_t*>(_deviceUserBase) + address;

    std::memcpy(data, targetAddress, sizeInBytes);
  }

  void UioDevice::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(bar > 0) {
      throw ChimeraTK::logic_error("Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map file are addressed using absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("Write request exceeds device memory region");
    }
    void* targetAddress = static_cast<uint8_t*>(_deviceUserBase) + address;

    std::memcpy(targetAddress, data, sizeInBytes);
  }

  std::string UioDevice::getDeviceFilePath() {
    return _deviceFilePath.string();
  }

  void UioDevice::UioMMap() {
    _deviceUserBase = mmap(NULL, _deviceMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _deviceFileDescriptor, 0);
    if(_deviceUserBase == MAP_FAILED) {
      ::close(_deviceFileDescriptor);
      throw ChimeraTK::runtime_error("Cannot allocate memory for UIO device '" + getDeviceFilePath() + "'");
    }
    return;
  }

  void UioDevice::UioUnmap() {
    munmap(_deviceUserBase, _deviceMemSize);
  }

  uint64_t UioDevice::readUint64HexFromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> std::hex >> value;
      inputFile.close();
    }
    return value;
  }
} // namespace ChimeraTK