// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/core/noncopyable.hpp>

#include <string>

namespace ChimeraTK {

  // Simple RAII wrapper for a device file
  class DeviceFile : private boost::noncopyable {
   public:
    DeviceFile() = delete;
    DeviceFile(std::string filePath, int flags);
    DeviceFile(DeviceFile&& d) noexcept;
    virtual ~DeviceFile();

    [[nodiscard]] std::string path() const;
    [[nodiscard]] int fd() const;

   private:
    [[nodiscard]] std::string _strerror(const std::string& msg) const;
    std::string _path{};
    int _fd{-1};
    int _savedErrno{0};
    bool _fdOwner{true};

  };

} // namespace ChimeraTK
