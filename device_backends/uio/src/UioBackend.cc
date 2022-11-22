// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioBackend.h"

#include <sys/mman.h>

#include <fcntl.h>
#include <fstream>

namespace ChimeraTK {

  UioBackend::UioBackend(std::string deviceName, std::string mapFileName)
  : NumericAddressedBackend(mapFileName), _deviceID(0), _deviceUserBase(nullptr),
    _deviceNodeName(std::string("/dev/" + deviceName)) {
    _deviceKernelBase = (void*)readUint64HexFromFile("/sys/class/uio/" + deviceName + "/maps/map0/addr");
    _deviceMemSize = readUint64HexFromFile("/sys/class/uio/" + deviceName + "/maps/map0/size");
  }

  UioBackend::~UioBackend() {
    close();
  }

  boost::shared_ptr<DeviceBackend> UioBackend::createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    if(address.size() == 0) {
      throw ChimeraTK::logic_error("Device Name not specified.");
    }

    return boost::shared_ptr<DeviceBackend>(new UioBackend(address, parameters["map"]));
  }

  void UioBackend::open() {
    if(_opened) {
      throw ChimeraTK::logic_error("Device already has been opened");
    }

    _deviceID = ::open(_deviceNodeName.c_str(), O_RDWR);
    if(_deviceID < 0) {
      throw ChimeraTK::runtime_error("Failed to open UIO device '" + _deviceNodeName + "'");
    }

    UioMMap();
    _opened = true;
  }

  void UioBackend::closeImpl() {
    if(_opened) {
      UioUnmap();
      ::close(_deviceID);
    }
    _opened = false;
  }

  bool UioBackend::isFunctional() const {
    return _opened;
  }

  void UioBackend::read(uint64_t /*bar*/, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(_opened == false) {
      throw ChimeraTK::logic_error("Device closed");
    }

    // This is a temporary work around, because register nodes of current map file are addressed using absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("Read request exceed Device Memory Region");
    }
    void* targetAddress = static_cast<uint8_t*>(_deviceUserBase) + address;

    std::memcpy(data, targetAddress, sizeInBytes);
  }

  void UioBackend::write(uint64_t /*bar*/, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(_opened == false) {
      throw ChimeraTK::logic_error("Device closed");
    }

    // This is a temporary work around, because register nodes of current map file are addressed using absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("Read request exceed Device Memory Region");
    }
    void* targetAddress = static_cast<uint8_t*>(_deviceUserBase) + address;

    std::memcpy(targetAddress, data, sizeInBytes);
  }

  std::string UioBackend::readDeviceInfo() {
    return std::string("UIO device backend");
  }

  uint64_t UioBackend::readUint64HexFromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> std::hex >> value;
      inputFile.close();
    }
    return value;
  }

  void UioBackend::UioMMap() {
    if(MAP_FAILED == (_deviceUserBase = mmap(NULL, _deviceMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _deviceID, 0))) {
      ::close(_deviceID);
      throw ChimeraTK::runtime_error("Cannot allocate Memory for UIO device '" + _deviceNodeName + "'");
    }
    return;
  }

  void UioBackend::UioUnmap() {
    munmap(_deviceUserBase, _deviceMemSize);
  }

} // namespace ChimeraTK