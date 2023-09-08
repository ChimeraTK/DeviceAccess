// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeviceFile.h"

#include "Exception.h"

#include <climits>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <utility>

namespace ChimeraTK {

  DeviceFile::DeviceFile(std::string deviceFilePath, int flags) : _path{std::move(deviceFilePath)} {
#ifdef _DEBUG
    std::cout << "MMIO: opening device file " << _path << std::endl;
#endif
    _fd = ::open(_path.c_str(), flags);
    _savedErrno = errno;
    if(_fd < 0) {
      throw runtime_error(_strerror("Cannot open device: "));
    }
  }

  DeviceFile::DeviceFile(DeviceFile&& d) noexcept : _path(std::move(d._path)), _fd(std::exchange(d._fd, 0)) {}

  DeviceFile::~DeviceFile() {
    if(_fd > 0) {
#ifdef _DEBUG
      std::cout << "MMIO: closing device file " << _path << std::endl;
#endif
      ::close(_fd);
    }
  }

  std::string DeviceFile::_strerror(const std::string& msg) const {
    char tmp[255];
    return msg + _path + ": " + ::strerror_r(_savedErrno, tmp, sizeof(tmp));
  }

  std::string DeviceFile::path() const {
    return _path;
  }

  int DeviceFile::fd() const {
    return _fd;
  }

} // namespace ChimeraTK
