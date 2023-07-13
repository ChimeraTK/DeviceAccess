// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "MmioAccess.h"
#include "XdmaIntfAbstract.h"

#include <string>

namespace ChimeraTK {

  // Control (config/status) interface to access the registers of FPGA IPs
  class CtrlIntf : public XdmaIntfAbstract {
    MmioAccess _mmio;

   public:
    CtrlIntf() = delete;
    explicit CtrlIntf(const std::string& devicePath);
    virtual ~CtrlIntf() = default;

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) override;
    void write(uintptr_t address, const int32_t* data, size_t nBytes) override;
  };

} // namespace ChimeraTK
