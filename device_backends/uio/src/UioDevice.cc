// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioDevice.h"

#include "Exception.h"
#include <sys/mman.h>

#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <poll.h>

namespace ChimeraTK {

  UioDevice::UioDevice(std::string deviceFilePath) : _deviceFilePath(deviceFilePath.c_str()) {
    std::string fileName = _deviceFilePath.filename().string();
    _deviceKernelBase = (void*)readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/addr");
    _deviceMemSize = readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/size");
    _lastInterruptCount = readUint32FromFile("/sys/class/uio/" + fileName + "/event");
  }

  UioDevice::~UioDevice() {
    close();
  }

  void UioDevice::open() {
    _deviceFileDescriptor = ::open(_deviceFilePath.c_str(), O_RDWR);
    if(_deviceFileDescriptor < 0) {
      throw ChimeraTK::runtime_error("Failed to open UIO device '" + getDeviceFilePath() + "'");
    }
    UioMMap();
    _opened = true;
  }

  void UioDevice::close() {
    if(!_opened) {
      UioUnmap();
      ::close(_deviceFileDescriptor);
      _opened = false;
    }
  }

  void UioDevice::read(uint64_t map, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(map > 0) {
      throw ChimeraTK::logic_error("UIO: Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map file are addressed using absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("UIO: Read request exceeds device memory region");
    }

    void* targetAddress = static_cast<uint8_t*>(_deviceUserBase) + address;

    std::memcpy(data, targetAddress, sizeInBytes);
  }

  void UioDevice::write(uint64_t map, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(map > 0) {
      throw ChimeraTK::logic_error("UIO: Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map file are addressed using absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("UIO: Write request exceeds device memory region");
    }
    void* targetAddress = static_cast<uint8_t*>(_deviceUserBase) + address;

    std::memcpy(targetAddress, data, sizeInBytes);
  }

  uint32_t UioDevice::waitForInterrupt(int timeoutMs) {
    // Represents the total interrupt count since system uptime.
    uint32_t totalInterruptCount = 0;
    // Will hold the number of new interrupts
    uint32_t occurredInterruptCount = 0;

    struct pollfd pfd;
    pfd.fd = _deviceFileDescriptor;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeoutMs);

    if(ret >= 1) {
      // No timeout, start reading
      ret = ::read(_deviceFileDescriptor, &totalInterruptCount, sizeof(totalInterruptCount));

      if(ret != (ssize_t)sizeof(totalInterruptCount)) {
        throw ChimeraTK::runtime_error("UIO - Reading interrupt failed: " + std::string(std::strerror(errno)));
      }

      // Prevent overflow of interrupt count value
      occurredInterruptCount = subtractUint32OverflowSafe(totalInterruptCount, _lastInterruptCount);
      _lastInterruptCount = totalInterruptCount;
    }
    else if(ret == 0) {
      // Timeout
      occurredInterruptCount = 0;
    }
    else {
      throw ChimeraTK::runtime_error("UIO - Waiting for interrupt failed: " + std::string(std::strerror(errno)));
    }

    return occurredInterruptCount;
  }

  void UioDevice::clearInterrupts() {
    uint32_t unmask = 1;
    ssize_t ret = ::write(_deviceFileDescriptor, &unmask, sizeof(unmask));

    if(ret != (ssize_t)sizeof(unmask)) {
      throw ChimeraTK::runtime_error("UIO - Waiting for interrupt failed: " + std::string(std::strerror(errno)));
    }
  }

  std::string UioDevice::getDeviceFilePath() {
    return _deviceFilePath.string();
  }

  void UioDevice::UioMMap() {
    _deviceUserBase = mmap(NULL, _deviceMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _deviceFileDescriptor, 0);
    if(_deviceUserBase == MAP_FAILED) {
      ::close(_deviceFileDescriptor);
      throw ChimeraTK::runtime_error("UIO: Cannot allocate memory for UIO device '" + getDeviceFilePath() + "'");
    }
    return;
  }

  void UioDevice::UioUnmap() {
    munmap(_deviceUserBase, _deviceMemSize);
  }

  uint32_t UioDevice::subtractUint32OverflowSafe(uint32_t minuend, uint32_t subtrahend) {
    if(subtrahend > minuend) {
      return minuend +
          (uint32_t)(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - static_cast<uint64_t>(subtrahend));
    }
    else {
      return minuend - subtrahend;
    }
  }

  uint32_t UioDevice::readUint32FromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> value;
      inputFile.close();
    }
    return (uint32_t)value;
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