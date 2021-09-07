#include "EventFile.h"

#include <fcntl.h>

#include <iostream>
#include <functional>

#include "XdmaBackend.h"

namespace ChimeraTK {
  EventThread::EventThread(int fd, size_t interruptIdx, XdmaBackend& receiver)
  : _svc{}, _sd{_svc, fd}, _interruptIdx(interruptIdx), _receiver(receiver), _thread(&EventThread::run, this) {
#ifdef _DEBUG
    std::cout << "XDMA: EventThread " << _interruptIdx << " ctor\n";
#endif
  }

  EventThread::~EventThread() {
#ifdef _DEBUG
    std::cout << "XDMA: EventThread " << _interruptIdx << " dtor\n";
#endif
    _svc.stop();
    _thread.join();
#ifdef _DEBUG
    std::cout << "XDMA: EventThread " << _interruptIdx << " dtor done\n";
#endif
  }

  void EventThread::run() {
#ifdef _DEBUG
    std::cout << "XDMA: EventThread(" << _interruptIdx << ")::run() start\n";
#endif
    waitForEvent();
    _svc.run();
#ifdef _DEBUG
    std::cout << "XDMA: EventThread(" << _interruptIdx << ")::run() end\n";
#endif
  }

  void EventThread::waitForEvent() {
#ifdef _DEBUG
    std::cout << "XDMA: Waiting for event " << _interruptIdx << "\n";
#endif
    _sd.async_read_some(boost::asio::buffer(_result),
        std::bind(&EventThread::handleEvent, this,
            std::placeholders::_1, // boost::asio::placeholders::error
            std::placeholders::_2  // boost::asio::placeholders::bytes_transferred
            ));
  }

  void EventThread::handleEvent(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if(ec) {
      const std::string msg = "EventThread I/O error: " + ec.message();
      throw runtime_error(msg);
    }
#ifdef _DEBUG
    std::cout << "XDMA: Event " << _interruptIdx << " received: " << bytes_transferred << " bytes, " << _result[0]
              << " interrupts\n";
#endif
    _receiver.dispatchInterrupt(0, _interruptIdx);
#ifdef _DEBUG
    std::cout << "XDMA: Event " << _interruptIdx << " dispatched\n";
#endif
    waitForEvent();
  }

  EventFile::EventFile(const std::string& devicePath, size_t interruptIdx, XdmaBackend& owner)
  : _file(devicePath + "/events" + std::to_string(interruptIdx), O_RDONLY), _interruptIdx(interruptIdx), _owner(owner) {
  }

  EventFile::~EventFile() { stopThread(); }

  void EventFile::startThread() {
    if(_evtThread) {
      return;
    }
#ifdef _DEBUG
    std::cout << "XDMA: Starting thread for event " << _interruptIdx << "\n";
#endif
    _evtThread = std::make_unique<EventThread>(_file, _interruptIdx, _owner);
#ifdef _DEBUG
    std::cout << "XDMA: Started thread for event " << _interruptIdx << "\n";
#endif
  }

  void EventFile::stopThread() {
    if(_evtThread) {
#ifdef _DEBUG
      std::cout << "XDMA: Stopping thread for event " << _interruptIdx << "\n";
#endif
      _evtThread.reset(nullptr);
    }
  }

} // namespace ChimeraTK