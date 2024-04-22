// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceFile.h"
#include "XdmaIntfAbstract.h"

#include <string>

namespace ChimeraTK {

  // DMA interface to access FPGA memory
  class DmaIntf : public XdmaIntfAbstract {
    DeviceFile _c2h;
    DeviceFile _h2c;

   public:
    DmaIntf() = delete;
    DmaIntf(const std::string& devicePath, size_t channelIdx);
    DmaIntf(DmaIntf&& d) = default; // Need move ctor for storage in std::vector
    virtual ~DmaIntf();

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nbytes) override;
    void write(uintptr_t address, const int32_t* data, size_t nbytes) override;
  };

} // namespace ChimeraTK
