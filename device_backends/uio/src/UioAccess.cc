// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioAccess.h"

#include "Exception.h"
#include <sys/mman.h>

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <poll.h>

namespace ChimeraTK {

  UioAccess::UioAccess(const std::string& deviceFilePath) : _deviceFilePath(deviceFilePath.c_str()) {
    std::string fileName = _deviceFilePath.filename().string();
    _deviceKernelBase = (void*)readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/addr");
    _deviceMemSize = readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/size");
    _lastInterruptCount = readUint32FromFile("/sys/class/uio/" + fileName + "/event");

    // Open UIO device file here, so that interrupt thread can run before calling open()
    _deviceFileDescriptor = ::open(_deviceFilePath.c_str(), O_RDWR);
    if(_deviceFileDescriptor < 0) {
      throw ChimeraTK::runtime_error("UIO: Failed to open device file '" + getDeviceFilePath() + "'");
    }
  }

  UioAccess::~UioAccess() {
    close();
  }

  void UioAccess::open() {
    UioMMap();
    _opened = true;
  }

  void UioAccess::close() {
    if(!_opened) {
      UioUnmap();
      ::close(_deviceFileDescriptor);
      _opened = false;
    }
  }

  void UioAccess::read(uint64_t map, uint64_t address, int32_t* __restrict__ data, size_t sizeInBytes) {
    if(map > 0) {
      throw ChimeraTK::logic_error("UIO: Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map use absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("UIO: Read request exceeds device memory region");
    }

    volatile int32_t* rptr = static_cast<volatile int32_t*>(_deviceUserBase) + address / sizeof(int32_t);
    while(sizeInBytes >= sizeof(int32_t)) {
      *(data++) = *(rptr++);
      sizeInBytes -= sizeof(int32_t);
    }
  }

  void UioAccess::write(uint64_t map, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(map > 0) {
      throw ChimeraTK::logic_error("UIO: Multiple memory regions are not supported");
    }

    // This is a temporary work around, because register nodes of current map use absolute bus addresses.
    address = address % reinterpret_cast<uint64_t>(_deviceKernelBase);

    if(address + sizeInBytes > _deviceMemSize) {
      throw ChimeraTK::logic_error("UIO: Write request exceeds device memory region");
    }

    volatile int32_t* __restrict__ wptr = static_cast<volatile int32_t*>(_deviceUserBase) + address / sizeof(int32_t);
    while(sizeInBytes >= sizeof(int32_t)) {
      *(wptr++) = *(data++);
      sizeInBytes -= sizeof(int32_t);
    }
  }

  uint32_t UioAccess::waitForInterrupt(int timeoutMs) {
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

  void UioAccess::clearInterrupts() {
    uint32_t unmask = 1;
    ssize_t ret = ::write(_deviceFileDescriptor, &unmask, sizeof(unmask));

    if(ret != (ssize_t)sizeof(unmask)) {
      throw ChimeraTK::runtime_error("UIO - Waiting for interrupt failed: " + std::string(std::strerror(errno)));
    }
  }

  std::string UioAccess::getDeviceFilePath() {
    return _deviceFilePath.string();
  }

  void UioAccess::UioMMap() {
    _deviceUserBase = mmap(NULL, _deviceMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _deviceFileDescriptor, 0);
    if(_deviceUserBase == MAP_FAILED) {
      ::close(_deviceFileDescriptor);
      throw ChimeraTK::runtime_error("UIO: Cannot allocate memory for UIO device '" + getDeviceFilePath() + "'");
    }
    return;
  }

  void UioAccess::UioUnmap() {
    munmap(_deviceUserBase, _deviceMemSize);
  }

  uint32_t UioAccess::subtractUint32OverflowSafe(uint32_t minuend, uint32_t subtrahend) {
    if(subtrahend > minuend) {
      return minuend +
          (uint32_t)(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - static_cast<uint64_t>(subtrahend));
    }
    else {
      return minuend - subtrahend;
    }
  }

  uint32_t UioAccess::readUint32FromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> value;
      inputFile.close();
    }
    return (uint32_t)value;
  }

  uint64_t UioAccess::readUint64HexFromFile(std::string fileName) {
    uint64_t value = 0;
    std::ifstream inputFile(fileName);

    if(inputFile.is_open()) {
      inputFile >> std::hex >> value;
      inputFile.close();
    }
    return value;
  }
} // namespace ChimeraTK
