// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioBackend.h"

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
      throw ChimeraTK::logic_error("Device Name not specified.");
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
      throw ChimeraTK::runtime_error("previous, unrecovered fault");
    }

    _uioDevice->read(bar, address, data, sizeInBytes);
  }

  void UioBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    assert(_opened);
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("previous, unrecovered fault");
    }

    _uioDevice->write(bar, address, data, sizeInBytes);
  }

  std::string UioBackend::readDeviceInfo() {
    if(!_opened) throw ChimeraTK::logic_error("Device not opened.");
    return std::string("UIO device backend: Device path = " + _uioDevice->getDeviceFilePath());
  }

} // namespace ChimeraTK