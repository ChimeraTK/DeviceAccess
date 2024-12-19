// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "async/DomainImpl.h"
#include "DeviceFile.h"

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace ChimeraTK {

  class EventFile;
  class EventThread {
    EventFile& _owner;

    boost::asio::io_context _ctx;
    boost::asio::posix::stream_descriptor _sd;
    std::thread _thread;

    std::array<uint32_t, 1> _result;

   public:
    EventThread() = delete;
    EventThread(EventFile& owner, std::promise<void> subscriptionDonePromise);
    ~EventThread();

    void start(std::promise<void> subscriptionDonePromise);
    void waitForEvent();
    void readEvent(const boost::system::error_code& ec);
    void handleEvent(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void timerEvent(const boost::system::error_code& ec);
    boost::asio::steady_timer timer{_ctx};
    const int timerSleepSec = 1;
  };

  // Event files are device files that are used to signal interrupt events to userspace
  class EventFile {
    friend class EventThread;
    DeviceBackend* _backend; // needed for reporting exceptions
    DeviceFile _file;
    boost::shared_ptr<async::DomainImpl<std::nullptr_t>> _asyncDomain;

    std::unique_ptr<EventThread> _evtThread;

   public:
    EventFile() = delete;
    EventFile(DeviceBackend* backend, const std::string& devicePath, size_t interruptIdx,
        boost::shared_ptr<async::DomainImpl<std::nullptr_t>> asyncDomain);
    // EventFile(EventFile&& d) = default;
    ~EventFile();

    void startThread(std::promise<void> subscriptionDonePromise);
  };

} // namespace ChimeraTK
