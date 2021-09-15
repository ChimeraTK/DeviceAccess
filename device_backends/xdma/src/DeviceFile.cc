#include "DeviceFile.h"

#include <iostream>
#include <utility>

#include <cstring>
#include <fcntl.h>

#include "Exception.h"

namespace ChimeraTK {

  DeviceFile::DeviceFile(const std::string& deviceFilePath, int flags)
  : _path{deviceFilePath}, _fd{::open(_path.c_str(), flags)} {
#ifdef _DEBUG
    std::cout << "XDMA: opening device file " << _path << std::endl;
#endif
    if(_fd < 0) {
      throw runtime_error(_strerror("Cannot open device: "));
    }
  }

  DeviceFile::DeviceFile(DeviceFile&& d) : _path(std::move(d._path)), _fd(std::exchange(d._fd, 0)) {}

  DeviceFile::~DeviceFile() {
    if(_fd > 0) {
#ifdef _DEBUG
      std::cout << "XDMA: closing device file " << _path << std::endl;
#endif
      ::close(_fd);
    }
  }

  std::string DeviceFile::_strerror(const std::string& msg) const {
    char tmp[255];
    return msg + _path + ": " + ::strerror_r(errno, tmp, sizeof(tmp));
  }

  DeviceFile::operator int() const { return _fd; }

  std::string DeviceFile::name() const { return _path; }

} // namespace ChimeraTK
