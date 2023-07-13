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
    MmioAccess(int fd, std::size_t mapSize, bool takeFdOwnership = true);
    ~MmioAccess();

    void read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes);
    void write(uintptr_t address, const int32_t* data, size_t nBytes);

   private:
    DeviceFile _file;
    void* _mem{nullptr};

    // Size of mmap'ed area in PCI BAR
    size_t _mmapSize{0};

    // 4 KiB is the minimum size available in Vivado
    static constexpr size_t _mmapSizeMin = 4UL * 1024UL;
    static constexpr size_t _mmapSizeMax = 16UL * 1024UL * 1024UL;

    [[nodiscard]] volatile int32_t* _reg_ptr(uintptr_t offs) const;
    void _check_range(const std::string& access_type, uintptr_t address, size_t nBytes) const;
    void mapFile(size_t mmapSize);

};

} // namespace ChimeraTK
