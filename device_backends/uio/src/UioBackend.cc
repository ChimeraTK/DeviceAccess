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
    setOpenedAndClearException();
  }

  void UioBackend::closeImpl() {
    if(_opened) {
      _uioAccess->close();
    }
    _opened = false;
  }

  bool UioBackend::barIndexValid(uint64_t bar) {
    if(bar != 0) return false;

    return true;
  }

  void UioBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    assert(_opened);
    checkActiveException();

    _uioAccess->read(bar, address, data, sizeInBytes);
  }

  void UioBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    assert(_opened);
    checkActiveException();

    _uioAccess->write(bar, address, data, sizeInBytes);
  }

  void UioBackend::startInterruptHandlingThread(uint32_t interruptNumber) {
    if(interruptNumber != 0) {
      throw ChimeraTK::logic_error("UIO: Backend only uses interrupt number 0");
    }

    if(_launchThreadMutex.try_lock()) {
      /* Mutex was found to be unlocked: Mutex is now locked so that handling thread will
          be execute once and run until the destructor is called. */
      _interruptWaitingThread = std::thread(&UioBackend::waitForInterruptLoop, this);
    }
  }

  std::string UioBackend::readDeviceInfo() {
    std::string result = std::string("UIO backend: Device path = " + _uioAccess->getDeviceFilePath());
    if(!isOpen()) {
      result += " (device closed)";
    }

    return result;
  }

  void UioBackend::waitForInterruptLoop() {
    uint32_t numberOfInterrupts;

    _uioAccess->clearInterrupts();

    while(!_stopInterruptLoop) {
      try {
        numberOfInterrupts = _uioAccess->waitForInterrupt(100);
      }
      catch(ChimeraTK::runtime_error& ex) {
        setException(ex.what());
        continue;
      }

      if(numberOfInterrupts > 0) {
        _uioAccess->clearInterrupts();

        if(!isFunctional()) {
          // Don't dispatch interrupts in case of exceptions
          continue;
        }

        for(uint32_t i = 0; i < numberOfInterrupts; i++) {
          dispatchInterrupt(0);
        }
      }
    }
  }

} // namespace ChimeraTK
