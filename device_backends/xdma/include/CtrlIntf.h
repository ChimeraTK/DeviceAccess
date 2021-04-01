#pragma once

#include <string>

#include "DeviceFile.h"
#include "XdmaIntfAbstract.h"

namespace ChimeraTK {

// Control (config/status) interface to access the registers of FPGA IPs
class CtrlIntf : public XdmaIntfAbstract {
    DeviceFile _file;
    void *_mem;
    size_t _mmapSize;

    volatile int32_t *_reg_ptr(uintptr_t offs) const;

public:
    CtrlIntf(const std::string &devicePath, uintptr_t mmapOffs, size_t mmapSize);
    virtual ~CtrlIntf();

    void read(uintptr_t address, int32_t* buf, size_t nBytes) override;
    void write(uintptr_t address, const int32_t* data, size_t nBytes) override;
};

}
