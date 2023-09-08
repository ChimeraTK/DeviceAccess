// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceFile.h"

#include <string>

namespace ChimeraTK {

  // Control (config/status) interface to access the registers of FPGA IPs
  class MmioAccess {
   public:
    MmioAccess() = delete;
    MmioAccess(const std::string& devicePath, std::size_t mapSize);
    ~MmioAccess();

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes);
    void write(uintptr_t address, const int32_t* data, size_t nBytes);

    [[nodiscard]] DeviceFile& getFile() { return _file; }

   private:
    DeviceFile _file;
    void* _mem{nullptr};

    // Size of mmap'ed area
    size_t _mmapSize{0};

    [[nodiscard]] volatile int32_t* _reg_ptr(uintptr_t offs) const;
    void _check_range(const std::string& access_type, uintptr_t address, size_t nBytes) const;
    void doMemoryMapping(size_t mmapSize);

};

} // namespace ChimeraTK
