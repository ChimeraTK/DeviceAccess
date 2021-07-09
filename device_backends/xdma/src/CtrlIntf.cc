#include "CtrlIntf.h"

#include <fcntl.h>
#include <sys/mman.h>

#include "DeviceFile.h"

namespace ChimeraTK {

  CtrlIntf::CtrlIntf(const std::string& devicePath) : _file(devicePath + "/user", O_RDWR) {
    _mem = ::mmap(NULL, _mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _file, 0);
  }

  CtrlIntf::~CtrlIntf() { ::munmap(_mem, _mmapSize); }

  volatile int32_t* CtrlIntf::_reg_ptr(uintptr_t offs) const { return static_cast<volatile int32_t*>(_mem) + offs / 4; }

  void CtrlIntf::read(uintptr_t address, int32_t* __restrict__ buf, size_t nbytes) {
    volatile int32_t* rptr = _reg_ptr(address);
    while(nbytes >= sizeof(int32_t)) {
      *buf++ = *rptr++;
      nbytes -= sizeof(int32_t);
    }
  }

  void CtrlIntf::write(uintptr_t address, const int32_t* data, size_t nbytes) {
    volatile int32_t* __restrict__ wptr = _reg_ptr(address);
    while(nbytes >= sizeof(int32_t)) {
      *wptr++ = *data++;
      nbytes -= sizeof(int32_t);
    }
  }

} // namespace ChimeraTK
