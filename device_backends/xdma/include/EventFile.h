#pragma once

#include "DeviceFile.h"

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace ChimeraTK {
  using EventCallback = std::function<void()>;

  class EventFile;
  class EventThread {
    EventFile& _owner;

    boost::asio::io_context _ctx;
    boost::asio::posix::stream_descriptor _sd;
    std::thread _thread;

    std::array<uint32_t, 1> _result;

   public:
    EventThread() = delete;
    EventThread(EventFile& owner);
    ~EventThread();

    void start();
    void waitForEvent();
    void readEvent(const boost::system::error_code& ec);
    void handleEvent(const boost::system::error_code& ec, std::size_t bytes_transferred);
  };

  // Event files are device files that are used to signal interrupt events to userspace
  class EventFile {
    friend class EventThread;
    DeviceFile _file;
    EventCallback _callback;

    std::unique_ptr<EventThread> _evtThread;

   public:
    EventFile() = delete;
    EventFile(const std::string& devicePath, size_t interruptIdx, EventCallback callback);
    EventFile(EventFile&& d) = default;
    ~EventFile();

    void startThread();
  };

} // namespace ChimeraTK
