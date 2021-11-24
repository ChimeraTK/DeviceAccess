#pragma once

#include <string>

#include "DeviceFile.h"
#include "XdmaIntfAbstract.h"

namespace ChimeraTK {

  // Control (config/status) interface to access the registers of FPGA IPs
  class CtrlIntf : public XdmaIntfAbstract {
    DeviceFile _file;
    void* _mem;

    // Map whole PCIe BAR area (16 MiB)
    static constexpr size_t _mmapSize = 16 * (1024 * 1024);

    volatile int32_t* _reg_ptr(uintptr_t offs) const;

   public:
    CtrlIntf() = delete;
    CtrlIntf(const std::string& devicePath);
    virtual ~CtrlIntf();

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) override;
    void write(uintptr_t address, const int32_t* data, size_t nBytes) override;
  };

} // namespace ChimeraTK
