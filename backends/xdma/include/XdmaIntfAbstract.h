// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

namespace ChimeraTK {

  class XdmaIntfAbstract {
   public:
    virtual void read(uintptr_t address, int32_t* __restrict__ buf, size_t nbytes) = 0;
    virtual void write(uintptr_t address, const int32_t* data, size_t nbytes) = 0;
  };

} // namespace ChimeraTK
