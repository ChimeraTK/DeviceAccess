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
      close();
    }

    _ctrlIntf.emplace(_devicePath);

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

    // (Re-)Open the event files which are needed
    std::for_each(_eventFiles.begin(), _eventFiles.end(), [](auto& eventFile) { eventFile = nullptr; });
    for(size_t i = 0; i < _maxInterrupts; i++) {
      if(_startInterruptHandlingCalled[i]) {
        _eventFiles[i] =
            std::make_unique<EventFile>(_devicePath, i, std::bind(&XdmaBackend::dispatchInterrupt, this, i));
        _eventFiles[i]->startThread();
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
  }

  bool XdmaBackend::isOpen() {
    return _ctrlIntf.has_value();
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

  void XdmaBackend::startInterruptHandlingThread(uint32_t interruptNumber) {
    if(interruptNumber >= _maxInterrupts) {
      throw ChimeraTK::logic_error("XDMA interrupt " + std::to_string(interruptNumber) + " out of range, only 0.." +
          std::to_string(_maxInterrupts - 1) + " available\n");
    }
    _startInterruptHandlingCalled[interruptNumber] = true;
    if(!isOpen()) {
      return;
    }

    if(!_eventFiles[interruptNumber]) {
      _eventFiles[interruptNumber] = std::make_unique<EventFile>(
          _devicePath, interruptNumber, std::bind(&XdmaBackend::dispatchInterrupt, this, interruptNumber));
      _eventFiles[interruptNumber]->startThread();
    }
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
