#include "EventFile.h"

#include <fcntl.h>

#include <iostream>
#include <functional>

#include "XdmaBackend.h"

namespace ChimeraTK {
  EventThread::EventThread(int fd, size_t interruptIdx, XdmaBackend& receiver)
  : _svc{}, _sd{_svc, fd}, _interruptIdx(interruptIdx), _receiver(receiver), _thread(&EventThread::waitForEvent, this) {
  }

  EventThread::~EventThread() {
    _svc.stop();
    _thread.join();
  }

  void EventThread::waitForEvent() {
#ifdef _DEBUG
    std::cout << "XDMA: Waiting for event...\n";
#endif
    _sd.async_read_some(boost::asio::null_buffers(), // No actual reading
        std::bind(&EventThread::handleEvent, this,
            std::placeholders::_1 // boost::asio::placeholders::error
            ));
  }

  void EventThread::handleEvent(const boost::system::error_code& ec) {
    if(ec) {
      const std::string msg = "EventThread I/O error: " + ec.message();
      throw runtime_error(msg);
    }
#ifdef _DEBUG
    std::cout << "XDMA: Event received.\n";
#endif
    _receiver.dispatchInterrupt(0, _interruptIdx);
    waitForEvent();
  }

  EventFile::EventFile(const std::string& devicePath, size_t interruptIdx, XdmaBackend& owner)
  : _file(devicePath + "/events" + std::to_string(interruptIdx), O_RDONLY), _owner(owner) {}

  EventFile::~EventFile() { stopThread(); }

  void EventFile::startThread() {
    if(_evtThread) {
      return;
    }
    _evtThread = std::make_unique<EventThread>(_file, _interruptIdx, _owner);
  }

  void EventFile::stopThread() {
    if(_evtThread) {
      _evtThread.reset(nullptr);
    }
  }

} // namespace ChimeraTK