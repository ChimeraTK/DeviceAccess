// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MmioAccess.h"

#include "Exception.h"
#include <sys/mman.h>

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>

// 4 KiB is the minimum size available we support for autodetection of mmap area size
static constexpr size_t _mmapAutodetectSizeMin = 4UL * 1024UL;
static constexpr size_t _mmapAutodetectSizeMax = 16UL * 1024UL * 1024UL;

namespace ChimeraTK {

  MmioAccess::MmioAccess(const std::string& devicePath, size_t mmapSize) : _file(devicePath, O_RDWR) {
    doMemoryMapping(mmapSize);
  }

  void MmioAccess::doMemoryMapping(size_t mmapSize) {
    int savedErrno = 0;
    // Auto-detect mapable size by trying to map the file until it works
    if(mmapSize == 0) {
      for(_mmapSize = _mmapAutodetectSizeMax; _mmapSize >= _mmapAutodetectSizeMin; _mmapSize /= 2) {
        errno = 0;
        _mem = ::mmap(nullptr, _mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _file.fd(), 0);
        savedErrno = errno;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        if(_mem != reinterpret_cast<void*>(-1)) {
          // Successfully mapped the BAR
#ifdef _DEBUG
          std::cout << "MMIO: mapped " << _mmapSize << " bytes" << std::endl;
#endif
          return;
        }
      }
    }
    else {
      _mmapSize = mmapSize;
      errno = 0;
      _mem = ::mmap(nullptr, _mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _file.fd(), 0);
      savedErrno = errno;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      if(_mem != reinterpret_cast<void*>(-1)) {
        // Successfully mapped the BAR
#ifdef _DEBUG
        std::cout << "MMIO: mapped " << _mmapSize << " bytes" << std::endl;
#endif
        return;
      }
    }
    char mmap_err[100];
    std::stringstream err_msg;
    err_msg << "MMIO: Failed to map " << _file.path() << ": " << strerror_r(savedErrno, mmap_err, sizeof(mmap_err));
    throw ChimeraTK::runtime_error(err_msg.str());
  }

  MmioAccess::MmioAccess(int fd, std::size_t mmapSize, bool takeFdOwnership) : _file(fd, takeFdOwnership) {
    doMemoryMapping(mmapSize);
  }

  MmioAccess::~MmioAccess() {
    ::munmap(_mem, _mmapSize);
  }

  volatile int32_t* MmioAccess::_reg_ptr(uintptr_t offs) const {
    return static_cast<volatile int32_t*>(_mem) + offs / 4;
  }

  void MmioAccess::_check_range(const std::string& access_type, uintptr_t address, size_t nBytes) const {
    if((address + nBytes) <= _mmapSize) {
      return;
    }
    std::stringstream err_msg;
    err_msg << "MMIO: attempt to " << access_type << " beyond mapped area: " << nBytes << " bytes at 0x" << std::hex
            << address << std::dec << " (" << _mmapSize << " bytes mapped)";
    throw ChimeraTK::runtime_error(err_msg.str());
  }

  void MmioAccess::read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) {
    _check_range("read", address, nBytes);
    volatile int32_t* rptr = _reg_ptr(address);
    while(nBytes >= sizeof(int32_t)) {
      *buf++ = *rptr++;
      nBytes -= sizeof(int32_t);
    }
  }

  void MmioAccess::write(uintptr_t address, const int32_t* data, size_t nBytes) {
    _check_range("write", address, nBytes);
    volatile int32_t* __restrict__ wptr = _reg_ptr(address);
    while(nBytes >= sizeof(int32_t)) {
      *wptr++ = *data++;
      nBytes -= sizeof(int32_t);
    }
  }

} // namespace ChimeraTK
