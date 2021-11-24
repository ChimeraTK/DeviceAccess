#pragma once

#include <string>

#include "DeviceFile.h"
#include "XdmaIntfAbstract.h"

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
