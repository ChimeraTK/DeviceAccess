#include "CtrlIntf.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sstream>

#include "DeviceFile.h"
#include "Exception.h"

namespace ChimeraTK {

  CtrlIntf::CtrlIntf(const std::string& devicePath) : _file(devicePath + "/user", O_RDWR) {
    _mem = ::mmap(NULL, _mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _file, 0);
  }

  CtrlIntf::~CtrlIntf() { ::munmap(_mem, _mmapSize); }

  volatile int32_t* CtrlIntf::_reg_ptr(uintptr_t offs) const { return static_cast<volatile int32_t*>(_mem) + offs / 4; }

  void CtrlIntf::_check_range(const std::string access_type, uintptr_t address, size_t nBytes) const {
    if((address + nBytes) <= _mmapSize) {
      return;
    }
    std::stringstream err_msg;
    err_msg << "XDMA: attempt to " << access_type << " beyond mapped area: " << nBytes << " bytes at 0x" << std::hex
            << address << std::dec;
    throw ChimeraTK::runtime_error(err_msg.str());
  }

  void CtrlIntf::read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) {
    _check_range("read", address, nBytes);
    volatile int32_t* rptr = _reg_ptr(address);
    while(nBytes >= sizeof(int32_t)) {
      *buf++ = *rptr++;
      nBytes -= sizeof(int32_t);
    }
  }

  void CtrlIntf::write(uintptr_t address, const int32_t* data, size_t nBytes) {
    _check_range("write", address, nBytes);
    volatile int32_t* __restrict__ wptr = _reg_ptr(address);
    while(nBytes >= sizeof(int32_t)) {
      *wptr++ = *data++;
      nBytes -= sizeof(int32_t);
    }
  }

} // namespace ChimeraTK
