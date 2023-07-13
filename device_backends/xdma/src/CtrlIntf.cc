// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CtrlIntf.h"

namespace ChimeraTK {

  CtrlIntf::CtrlIntf(const std::string& devicePath) : _mmio(devicePath + "/user", 0) {}

  void CtrlIntf::read(uintptr_t address, int32_t* __restrict__ buf, size_t nBytes) {
    _mmio.read(address, buf, nBytes);
  }

  void CtrlIntf::write(uintptr_t address, const int32_t* data, size_t nBytes) {
    _mmio.write(address, data, nBytes);
  }

} // namespace ChimeraTK
