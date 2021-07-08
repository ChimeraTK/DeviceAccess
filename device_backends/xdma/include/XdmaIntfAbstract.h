#pragma once

#include <cstdint>
#include <cstddef>

namespace ChimeraTK {

  class XdmaIntfAbstract {
   public:
    virtual void read(uintptr_t address, int32_t* __restrict__ buf, size_t nbytes) = 0;
    virtual void write(uintptr_t address, const int32_t* data, size_t nbytes) = 0;
  };

} // namespace ChimeraTK
