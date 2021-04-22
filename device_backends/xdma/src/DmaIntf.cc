#include "DmaIntf.h"

#include <fcntl.h>

#include "DeviceFile.h"

namespace ChimeraTK {

DmaIntf::DmaIntf(const std::string &devicePath, size_t channelIdx)
:_c2h(devicePath + "/c2h" + std::to_string(channelIdx), O_RDONLY)
,_h2c(devicePath + "/h2c" + std::to_string(channelIdx), O_WRONLY) {
}

DmaIntf::DmaIntf(DmaIntf &&d)
:_c2h(std::move(d._c2h))
,_h2c(std::move(d._h2c)) {
}

DmaIntf::~DmaIntf() {
}

void DmaIntf::read(uintptr_t address, int32_t* __restrict__ buf, size_t nbytes) {
    ssize_t result = ::pread(_c2h, buf, nbytes, address);
    if (result != static_cast<ssize_t>(nbytes)) {
        // TODO: error handling
    }
}

void DmaIntf::write(uintptr_t address, const int32_t* data, size_t nbytes) {
    ssize_t result = ::pwrite(_h2c, data, nbytes, address);
    if (result != static_cast<ssize_t>(nbytes)) {
        // TODO: error handling
    }
}

}
