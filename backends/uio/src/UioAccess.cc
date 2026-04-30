// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioAccess.h"

#include "Exception.h"

#include <sys/mman.h>

#include <boost/filesystem/directory.hpp>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <format>
#include <fstream>
#include <limits>
#include <utility>

namespace ChimeraTK {

  UioAccess::UioMap::UioMap() {}

  UioAccess::UioMap::UioMap(int deviceFileDescriptor, size_t uioMapIdx, const std::string& uioMapPath)
  : _deviceLowerBound(readUint64HexFromFile(uioMapPath + "/addr")),
    _deviceHigherBound(_deviceLowerBound + readUint64HexFromFile(uioMapPath + "/size")) {
    size_t mapSize = _deviceHigherBound - _deviceLowerBound;

    void* mapped =
        mmap(NULL, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, deviceFileDescriptor, uioMapIdx * getpagesize());

    if(mapped == MAP_FAILED) {
      _deviceLowerBound = 0;
      _deviceHigherBound = 0;
      throw ChimeraTK::runtime_error("UIO: Cannot allocate memory for UIO map '" + uioMapPath + "'");
    }

    _deviceUserBase = mapped;
  }

  UioAccess::UioMap::~UioMap() {
    if(*this) {
      auto mapSize = _deviceHigherBound - _deviceLowerBound;
      munmap(_deviceUserBase, mapSize);
    }
  }

  UioAccess::UioMap::UioMap(UioMap&& other) noexcept
  : _deviceLowerBound(std::exchange(other._deviceLowerBound, 0)),
    _deviceHigherBound(std::exchange(other._deviceHigherBound, 0)),
    _deviceUserBase(std::exchange(other._deviceUserBase, nullptr)) {}

  UioAccess::UioMap& UioAccess::UioMap::operator=(UioMap&& other) noexcept {
    if(this != &other) {
      if(*this) {
        auto mapSize = _deviceHigherBound - _deviceLowerBound;
        munmap(_deviceUserBase, mapSize);
      }

      this->_deviceLowerBound = std::exchange(other._deviceLowerBound, 0);
      this->_deviceHigherBound = std::exchange(other._deviceHigherBound, 0);
      this->_deviceUserBase = std::exchange(other._deviceUserBase, nullptr);
    }
    return *this;
  }

  UioAccess::UioMap::operator bool() const noexcept {
    return _deviceUserBase != nullptr;
  }

  void UioAccess::UioMap::read(uint64_t address, int32_t* data, size_t sizeInBytes) {
    volatile int32_t* rptr = static_cast<volatile int32_t*>(_deviceUserBase) +
        checkMapAddress(address, sizeInBytes, false) / sizeof(int32_t);

    while(sizeInBytes >= sizeof(int32_t)) {
      *(data++) = *(rptr++);
      sizeInBytes -= sizeof(int32_t);
    }
  }

  void UioAccess::UioMap::write(uint64_t address, int32_t const* data, size_t sizeInBytes) {
    volatile int32_t* __restrict__ wptr =
        static_cast<volatile int32_t*>(_deviceUserBase) + checkMapAddress(address, sizeInBytes, true) / sizeof(int32_t);

    while(sizeInBytes >= sizeof(int32_t)) {
      *(wptr++) = *(data++);
      sizeInBytes -= sizeof(int32_t);
    }
  }

  size_t UioAccess::UioMap::checkMapAddress(uint64_t address, size_t sizeInBytes, bool isWrite) {
    if(!*this) [[unlikely]] {
      std::string requestType = isWrite ? "Write" : "Read";
      throw ChimeraTK::logic_error(std::format("UIO: {} request on unmapped memory region", requestType));
    }

    if(address < _deviceLowerBound || address + sizeInBytes > _deviceHigherBound) [[unlikely]] {
      std::string requestType = isWrite ? "Write" : "Read";
      throw ChimeraTK::logic_error(
          std::format("UIO: {} request (low = {}, high = {}) outside device memory region (low = {}, high = {})",
              requestType, address, address + sizeInBytes, _deviceLowerBound, _deviceHigherBound));
    }

    // This is a temporary work around, because register nodes of current map use absolute bus addresses.
    return address - _deviceLowerBound;
  }

  UioAccess::UioAccess(const std::string& deviceFilePath) : _deviceFilePath(deviceFilePath.c_str()) {}

  UioAccess::~UioAccess() {
    close();
  }

  void UioAccess::open() {
    if(boost::filesystem::is_symlink(_deviceFilePath)) {
      _deviceFilePath = boost::filesystem::canonical(_deviceFilePath);
    }
    _filename = _deviceFilePath.filename().string();
    _lastInterruptCount = readUint32FromFile("/sys/class/uio/" + _filename + "/event");

    // Open UIO device file here, so that interrupt thread can run before calling open()
    _deviceFileDescriptor = ::open(_deviceFilePath.c_str(), O_RDWR);
    if(_deviceFileDescriptor < 0) {
      throw ChimeraTK::runtime_error("UIO: Failed to open device file '" + getDeviceFilePath() + "'");
    }

    _maps_number = 0;
    while(true) {
      std::string uioMapPath = "/sys/class/uio/" + _filename + "/maps/map" + std::to_string(_maps_number);
      if(!boost::filesystem::is_directory(uioMapPath)) break;
      _maps_number++;
    }

    _opened = true;
  }

  void UioAccess::close() {
    if(_opened) {
      for(auto& map : _maps) map = UioMap{};
      ::close(_deviceFileDescriptor);
      _opened = false;
    }
  }

  bool UioAccess::mapIndexValid(uint64_t map) {
    return map < _maps_number;
  }

  void UioAccess::read(uint64_t map, uint64_t address, int32_t* __restrict__ data, size_t sizeInBytes) {
    if(!mapIndexValid(map)) [[unlikely]] {
      throw ChimeraTK::logic_error("UIO: Attempt to access map" + std::to_string(map) +
          " outside the range (registered maps = " + std::to_string(_maps.size()) + ")");
    }

    getMap(map).read(address, data, sizeInBytes);
  }

  void UioAccess::write(uint64_t map, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(!mapIndexValid(map)) [[unlikely]] {
      throw ChimeraTK::logic_error("UIO: Attempt to access map" + std::to_string(map) +
          " outside the range (registered maps = " + std::to_string(_maps.size()) + ")");
    }

    getMap(map).write(address, data, sizeInBytes);
  }

  UioAccess::UioMap& UioAccess::getMap(size_t map) {
    if(!_maps[map]) [[unlikely]] {
      std::string uioMapPath = "/sys/class/uio/" + _filename + "/maps/map" + std::to_string(map);
      _maps[map] = UioAccess::UioMap(_deviceFileDescriptor, map, uioMapPath);
    }

    return _maps[map];
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
