#include "CtrlIntf.h"

#include <fcntl.h>
#include <sys/mman.h>

#include "DeviceFile.h"

namespace ChimeraTK {

CtrlIntf::CtrlIntf(const std::string &devicePath, uintptr_t mmapOffs, size_t mmapSize)
:_file(devicePath + "/user", O_RDWR)
,_mmapSize(mmapSize) {
    _mem = ::mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _file, mmapOffs);
}

CtrlIntf::~CtrlIntf() {
    ::munmap(_mem, _mmapSize);
}

volatile int32_t *CtrlIntf::_reg_ptr(uintptr_t offs) const {
    return static_cast<volatile int32_t *>(_mem) + offs / 4;
}

void CtrlIntf::read(uintptr_t address, int32_t* buf, size_t nbytes) {
    while (nbytes >= sizeof(int32_t)) {
        *buf++ = *_reg_ptr(address);
        address += sizeof(int32_t);
        nbytes -= sizeof(int32_t);
    }
}

void CtrlIntf::write(uintptr_t address, const int32_t* data, size_t nbytes) {
    while (nbytes >= sizeof(int32_t)) {
        *_reg_ptr(address) = *data++;
        address += sizeof(int32_t);
        nbytes -= sizeof(int32_t);
    }
}

}
