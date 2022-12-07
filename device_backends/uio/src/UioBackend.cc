// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioBackend.h"

namespace ChimeraTK {

  UioBackend::UioBackend(std::string deviceName, std::string mapFileName) : NumericAddressedBackend(mapFileName) {
    _uioAccess = std::shared_ptr<UioAccess>(new UioAccess("/dev/" + deviceName));
  }

  UioBackend::~UioBackend() {
    closeImpl();

    if(_interruptWaitingThread.joinable()) {
      _stopInterruptLoop = true;
      _interruptWaitingThread.join();
    }
  }

  boost::shared_ptr<DeviceBackend> UioBackend::createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    if(address.size() == 0) {
      throw ChimeraTK::logic_error("UIO: Device name not specified.");
    }
    return boost::shared_ptr<DeviceBackend>(new UioBackend(address, parameters["map"]));
  }

  void UioBackend::open() {
    if(_opened) {
      if(isFunctional()) {
        return;
      }
      close();
    }

    _uioAccess->open();
    _hasActiveException = false;
    _opened = true;
  }

  void UioBackend::closeImpl() {
    if(_opened) {
      _uioAccess->close();
    }
    _opened = false;
  }

  bool UioBackend::isFunctional() const {
    if(!_opened) return false;
    if(_hasActiveException) return false;

    return true;
  }

  void UioBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    assert(_opened);
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("UIO: Previous, un-recovered fault");
    }

    _uioAccess->read(bar, address, data, sizeInBytes);
  }

  void UioBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    assert(_opened);
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("UIO: Previous, un-recovered fault");
    }

    _uioAccess->write(bar, address, data, sizeInBytes);
  }

  void UioBackend::startInterruptHandlingThread(unsigned int interruptControllerNumber, unsigned int interruptNumber) {
    if(interruptControllerNumber != 0) {
      throw ChimeraTK::logic_error("UIO: Backend only uses interrupt controller 0");
    }
    if(interruptNumber != 0) {
      throw ChimeraTK::logic_error("UIO: Backend only uses interrupt number 0");
    }

    if(_interruptThreadMutex.try_lock()) {
      _interruptWaitingThread = std::thread(&UioBackend::waitForInterruptThread, this);
    }
  }

  std::string UioBackend::readDeviceInfo() {
    std::string result = std::string("UIO backend: Device path = " + _uioAccess->getDeviceFilePath());
    if(!isOpen()) {
      result += " (device closed)";
    }

    return result;
  }

  void UioBackend::waitForInterruptThread() {
    std::lock_guard<std::mutex> lock(_interruptThreadMutex, std::adopt_lock);

    _uioAccess->clearInterrupts();

    while(!_stopInterruptLoop) {
      uint32_t numberOfInterrupts = _uioAccess->waitForInterrupt(100);

      if(numberOfInterrupts > 0) {
        _uioAccess->clearInterrupts();

        if(_hasActiveException) {
          // Don't dispatch interrupt in case of exceptions
          return;
        }

        for(uint32_t i = 0; i < numberOfInterrupts; i++) {
          dispatchInterrupt(0, 0);
        }
      }
    }
  }

} // namespace ChimeraTK