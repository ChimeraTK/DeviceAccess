// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "XdmaBackend.h"

#include <boost/make_shared.hpp>

#include <functional>
#include <iomanip>

namespace ChimeraTK {

  XdmaBackend::XdmaBackend(std::string devicePath, std::string mapFileName)
  : NumericAddressedBackend(mapFileName), _devicePath(devicePath) {}

  XdmaBackend::~XdmaBackend() {}

  void XdmaBackend::open() {
#ifdef _DEBUG
    std::cout << "XDMA: opening dev: " << _devicePath << std::endl;
#endif
    if(_ctrlIntf) {
      if(isFunctional()) {
        return;
      }
    }

    _ctrlIntf.emplace(_devicePath);

    std::for_each(_eventFiles.begin(), _eventFiles.end(), [](auto& eventFile) { eventFile = nullptr; });

    // Build vector of DMA channels
    _dmaChannels.clear();
    for(size_t i = 0; i < _maxDmaChannels; i++) {
      try {
        _dmaChannels.emplace_back(_devicePath, i);
      }
      catch(const runtime_error&) {
        break;
      }
    }

#ifdef _DEBUG
    std::cout << "XDMA: opened interface with " << _dmaChannels.size() << " DMA channels and " << _eventFiles.size()
              << " interrupt sources\n";
#endif
    setOpenedAndClearException();
  }

  void XdmaBackend::closeImpl() {
    std::for_each(_eventFiles.begin(), _eventFiles.end(), [](auto& eventFile) { eventFile = nullptr; });
    _ctrlIntf.reset();
    _dmaChannels.clear();
    _opened = false;
  }

  bool XdmaBackend::isOpen() {
    return _opened;
  }

  XdmaIntfAbstract& XdmaBackend::_intfFromBar(uint64_t bar) {
    if(bar == 0 && _ctrlIntf.has_value()) {
      return _ctrlIntf.value();
    }
    // 13 is magic value for DMA channel (by convention)
    // We provide N DMA channels starting from there
    if(bar >= 13) {
      const size_t dmaChIdx = bar - 13;
      if(dmaChIdx < _dmaChannels.size()) {
        return _dmaChannels[dmaChIdx];
      }
    }
    throw ChimeraTK::logic_error("Couldn't find XDMA channel for BAR value " + std::to_string(bar));
  }

#ifdef _DEBUG
  void XdmaBackend::dump(const int32_t* data, size_t nbytes) {
    constexpr size_t wordsPerLine = 8;
    size_t n;
    for(n = 0; n < (nbytes / sizeof(*data)) && n < 64; n++) {
      if(!(n % wordsPerLine)) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << n * sizeof(*data) << ":";
      }
      std::cout << " " << std::hex << std::setw(8) << std::setfill('0') << data[n];
      if((n % wordsPerLine) == wordsPerLine - 1) {
        std::cout << "\n";
      }
    }
    if((n % wordsPerLine) != wordsPerLine) {
      std::cout << "\n";
    }
  }
#endif

  void XdmaBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
#ifdef _DEBUGDUMP
    std::cout << "XDMA: read " << sizeInBytes << " bytes @ BAR" << bar << ", 0x" << std::hex << address << std::endl;
#endif
    auto& intf = _intfFromBar(bar);
    intf.read(address, data, sizeInBytes);
#ifdef _DEBUGDUMP
    dump(data, sizeInBytes);
#endif
  }

  void XdmaBackend::write(uint64_t bar, uint64_t address, const int32_t* data, size_t sizeInBytes) {
#ifdef _DEBUGDUMP
    std::cout << "XDMA: write " << sizeInBytes << " bytes @ BAR" << bar << ", 0x" << std::hex << address << std::endl;
#endif
    auto& intf = _intfFromBar(bar);
    intf.write(address, data, sizeInBytes);
#ifdef _DEBUGDUMP
    dump(data, sizeInBytes);
#endif
  }

  std::future<void> XdmaBackend::activateSubscription(
      uint32_t interruptNumber, boost::shared_ptr<async::DomainImpl<std::nullptr_t>> asyncDomain) {
    std::promise<void> subscriptionDonePromise;
    auto subscriptionDoneFuture = subscriptionDonePromise.get_future();
    if(interruptNumber >= _maxInterrupts) {
      setException("XDMA interrupt " + std::to_string(interruptNumber) + " out of range, only 0.." +
          std::to_string(_maxInterrupts - 1) + " available\n");
      subscriptionDonePromise.set_value();
      return subscriptionDoneFuture;
    }

    if(!_eventFiles[interruptNumber]) {
      _eventFiles[interruptNumber] = std::make_unique<EventFile>(_devicePath, interruptNumber, asyncDomain);
      _eventFiles[interruptNumber]->startThread(std::move(subscriptionDonePromise));
    }
    else {
      // thread is already running, just fulfil the promise
      subscriptionDonePromise.set_value();
    }
    return subscriptionDoneFuture;
  }

  std::string XdmaBackend::readDeviceInfo() {
    std::string result = "XDMA backend: Device path = " + _devicePath + ", number of DMA channels = ";
    if(isOpen()) {
      result += std::to_string(_dmaChannels.size());
    }
    else {
      result += "unknown (device closed)";
    }

    // TODO: retrieve other interesting stuff (driver version...) via ioctl
    return result;
  }

  boost::shared_ptr<DeviceBackend> XdmaBackend::createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    if(address.empty()) {
      throw ChimeraTK::logic_error("XDMA device address not specified.");
    }

    return boost::make_shared<XdmaBackend>("/dev/" + address, parameters["map"]);
  }

} // namespace ChimeraTK
