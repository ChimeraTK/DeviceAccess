// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DmaIntf.h"

#include "DeviceFile.h"
#include "Exception.h"

#include <fcntl.h>

namespace ChimeraTK {

  DmaIntf::DmaIntf(const std::string& devicePath, size_t channelIdx)
  : _c2h(devicePath + "/c2h" + std::to_string(channelIdx), O_RDONLY),
    _h2c(devicePath + "/h2c" + std::to_string(channelIdx), O_WRONLY) {}

  DmaIntf::~DmaIntf() {}

  void DmaIntf::read(uintptr_t address, int32_t* __restrict__ buf, size_t nbytes) {
    ssize_t result = ::pread(_c2h.fd(), buf, nbytes, address);
    if(result != static_cast<ssize_t>(nbytes)) {
      throw(ChimeraTK::runtime_error(
          "DmaIntf read size mismatch: read " + std::to_string(result) + " bytes, expected " + std::to_string(nbytes)));
    }
  }

  void DmaIntf::write(uintptr_t address, const int32_t* data, size_t nbytes) {
    ssize_t result = ::pwrite(_h2c.fd(), data, nbytes, address);
    if(result != static_cast<ssize_t>(nbytes)) {
      throw(ChimeraTK::runtime_error("DmaIntf write size mismatch: wrote " + std::to_string(result) +
          " bytes, expected " + std::to_string(nbytes)));
    }
  }

} // namespace ChimeraTK
