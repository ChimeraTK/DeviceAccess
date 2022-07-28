// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceFile.h"
#include "XdmaIntfAbstract.h"

#include <string>

namespace ChimeraTK {

  // Control (config/status) interface to access the registers of FPGA IPs
  class CtrlIntf : public XdmaIntfAbstract {
    DeviceFile _file;
    void* _mem;

    // Size of mmap'ed area in PCI BAR
    size_t _mmapSize;
    // 4 KiB is the minimum size available in Vivado
    static constexpr size_t _mmapSizeMin = 4 * 1024;
    static constexpr size_t _mmapSizeMax = 16 * 1024 * 1024;

    volatile int32_t* _reg_ptr(uintptr_t offs) const;
    void _check_range(const std::string access_type, uintptr_t address, size_t nBytes) const;

   public:
    CtrlIntf() = delete;
    CtrlIntf(const std::string& devicePath);
    virtual ~CtrlIntf();

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) override;
    void write(uintptr_t address, const int32_t* data, size_t nBytes) override;
  };

} // namespace ChimeraTK
