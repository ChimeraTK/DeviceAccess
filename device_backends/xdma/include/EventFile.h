#pragma once

#include <string>
#include <thread>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include "DeviceFile.h"

namespace ChimeraTK {
  class XdmaBackend;

  class EventThread {
    boost::asio::io_context _ctx;
    boost::asio::posix::stream_descriptor _sd;
    size_t _interruptIdx;
    XdmaBackend& _receiver;
    std::thread _thread;

    std::array<uint32_t, 1> _result;

   public:
    EventThread() = delete;
    EventThread(int fd, size_t interruptIdx, XdmaBackend& receiver);
    ~EventThread();

    void waitForEvent();
    void readEvent(const boost::system::error_code& ec);
    void handleEvent(const boost::system::error_code& ec, std::size_t bytes_transferred);
  };

  // Event files are device files that are used to signal interrupt events to userspace
  class EventFile {
    DeviceFile _file;
    size_t _interruptIdx;
    XdmaBackend& _owner;

    std::unique_ptr<EventThread> _evtThread;

   public:
    EventFile() = delete;
    EventFile(const std::string& devicePath, size_t interruptIdx, XdmaBackend& owner);
    EventFile(EventFile&& d) = default;
    ~EventFile();

    void startThread();
  };

} // namespace ChimeraTK
