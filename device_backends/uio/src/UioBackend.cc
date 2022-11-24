// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioBackend.h"

#include <iostream>

namespace ChimeraTK {

  UioBackend::UioBackend(std::string deviceName, std::string mapFileName) : NumericAddressedBackend(mapFileName) {
    _uioDevice = boost::shared_ptr<UioDevice>(new UioDevice("/dev/" + deviceName));
  }

  UioBackend::~UioBackend() {
    _uioDevice->close();
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
      if(isFunctional()) return;
      _uioDevice->close();
    }

    _uioDevice->open();

    _hasActiveException = false;
    _opened = true;
  }

  void UioBackend::closeImpl() {
    if(_opened) {
      _uioDevice->close();
    }
    _opened = false;
  }

  bool UioBackend::isFunctional() const {
    if(!_opened) return false;
    if(_hasActiveException) return false;

    // Do something meaningfull here, like non-blocking read of module ID (0x00)
    return true;
  }

  void UioBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    assert(_opened);
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("UIO: Previous, unrecovered fault");
    }

    _uioDevice->read(bar, address, data, sizeInBytes);
  }

  void UioBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    assert(_opened);
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("UIO: Previous, unrecovered fault");
    }

    _uioDevice->write(bar, address, data, sizeInBytes);
  }

  void UioBackend::startInterruptHandlingThread(unsigned int interruptControllerNumber, unsigned int interruptNumber) {
    if(!_opened) throw ChimeraTK::logic_error("UIO: Device not opened.");

    if(interruptControllerNumber != 0) {
      throw ChimeraTK::logic_error("UIO: Backend only uses interrupt controller 0");
    }
    if(interruptNumber != 0) {
      throw ChimeraTK::logic_error("UIO: Backend only uses interrupt number 0");
    }

    boost::unique_lock<boost::mutex> lock(_interruptThreadMutex, boost::try_to_lock);
    if(!lock.owns_lock()) {
      boost::thread th(boost::bind(&UioBackend::waitForInterruptThread, this));
      th.detach();
    }
  }

  std::string UioBackend::readDeviceInfo() {
    if(!_opened) throw ChimeraTK::logic_error("UIO: Device not opened.");
    return std::string("UIO device backend: Device path = " + _uioDevice->getDeviceFilePath());
  }

  void UioBackend::waitForInterruptThread() {
    boost::lock_guard<boost::mutex> lock(_interruptThreadMutex, boost::adopt_lock);

    int numberOfInterrupts = _uioDevice->waitForInterrupt(-1);
    _uioDevice->clearInterrupts();

    std::cout << "Received #" << numberOfInterrupts << " interrupts" << std::endl;

    for(int i = 0; i < numberOfInterrupts; i++) {
      dispatchInterrupt(0, 0);
    }
  }

} // namespace ChimeraTK