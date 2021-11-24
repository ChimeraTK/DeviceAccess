#pragma once

#include <boost/core/noncopyable.hpp>

#include <string>

namespace ChimeraTK {

  // Simple RAII wrapper for a device file
  class DeviceFile : private boost::noncopyable {
   private:
    std::string _strerror(const std::string& msg) const;
    const std::string _path;
    int _fd;

   public:
    DeviceFile() = delete;
    DeviceFile(const std::string& filePath, int flags);
    DeviceFile(DeviceFile&& d);
    virtual ~DeviceFile();

    operator int() const;
    std::string name() const;
  };

} // namespace ChimeraTK
