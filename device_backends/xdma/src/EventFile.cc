// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EventFile.h"

#include "Exception.h"

#include <iostream>

namespace io = boost::asio;

namespace ChimeraTK {
  EventThread::EventThread(EventFile& owner, std::promise<void> subscriptionDonePromise)
  : _owner{owner}, _ctx{}, _sd{_ctx, owner._file}, _thread{
                                                       &EventThread::start, this, std::move(subscriptionDonePromise)} {
#ifdef _DEBUG
    std::cout << "XDMA: EventThread " << _owner._file.name() << " ctor\n";
#endif
  }

  EventThread::~EventThread() {
#ifdef _DEBUG
    std::cout << "XDMA: EventThread " << _owner._file.name() << " dtor\n" << std::endl;
#endif
    _ctx.stop();
    _thread.join();
  }

  void EventThread::start(std::promise<void> subscriptionDonePromise) {
    // The thread has started, next thing is going to be the wait.
    // This is the time to fulfil the promise that the subscription is done.
    subscriptionDonePromise.set_value();
    waitForEvent();
    _ctx.run();
  }

  void EventThread::waitForEvent() {
#ifdef _DEBUG
    std::cout << "XDMA: waitForEvent " << _owner._file.name() << "\n";
#endif
    // We have to wait seperately from the read operation,
    // since the read op will not be canceled by _ctx.stop() in the dtor
    _sd.async_wait(io::posix::stream_descriptor::wait_read,
        std::bind(&EventThread::readEvent, this,
            std::placeholders::_1 // io::placeholders::error
            ));
  }

  void EventThread::readEvent(const boost::system::error_code& ec) {
#ifdef _DEBUG
    std::cout << "XDMA: readEvent " << _owner._file.name() << "\n";
#endif
    if(ec) {
      const std::string msg = "EventThread::readEvent() I/O error: " + ec.message();
      throw runtime_error(msg);
    }
    _sd.async_read_some(io::buffer(_result),
        std::bind(&EventThread::handleEvent, this,
            std::placeholders::_1, // io::placeholders::error
            std::placeholders::_2  // io::placeholders::bytes_transferred
            ));
  }

  void EventThread::handleEvent(const boost::system::error_code& ec, std::size_t bytes_transferred) {
#ifdef _DEBUG
    std::cout << "XDMA: handleEvent " << _owner._file.name() << "\n";
#endif
    if(ec) {
      const std::string msg = "EventThread::handleEvent() I/O error: " + ec.message();
      throw runtime_error(msg);
    }
    if(bytes_transferred != sizeof(_result[0])) {
      throw runtime_error("EventThread::handleEvent() incomplete read");
    }

    uint32_t numInterrupts = _result[0];
#ifdef _DEBUG
    std::cout << "XDMA: Event " << _owner._file.name() << " received: " << bytes_transferred << " bytes, "
              << numInterrupts << " interrupts\n";
#endif
    while(numInterrupts--) {
      _owner._callback();
    }
    waitForEvent();
  }

  EventFile::EventFile(const std::string& devicePath, size_t interruptIdx, EventCallback callback)
  : _file{devicePath + "/events" + std::to_string(interruptIdx), O_RDONLY}, _callback{callback} {}

  EventFile::~EventFile() {
    _evtThread.reset(nullptr);
  }

  void EventFile::startThread(std::promise<void> subscriptionDonePromise) {
    if(_evtThread) {
      subscriptionDonePromise.set_value();
      return;
    }
    _evtThread = std::make_unique<EventThread>(*this, std::move(subscriptionDonePromise));
  }

} // namespace ChimeraTK
