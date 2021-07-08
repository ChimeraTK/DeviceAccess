#include "XdmaBackend.h"

#include <boost/make_shared.hpp>
#include <iomanip>

namespace ChimeraTK {

  XdmaBackend::XdmaBackend(std::string devicePath, std::string mapFileName)
  : NumericAddressedBackend(mapFileName), _devicePath(devicePath) {}

  XdmaBackend::~XdmaBackend() {}

  void XdmaBackend::open() {
#ifdef _DEBUG
    std::cout << "open xdma dev: " << _devicePath << std::endl;
#endif
    if(_ctrlIntf) {
      if(isFunctional()) {
        return;
      }
      close();
    }
    // TODO: retrieve mmap_(offs,size) from map file
    _ctrlIntf.emplace(_devicePath);

    // Build vector of max. 4 DMA channels
    _dmaChannels.clear();
    for(size_t i = 0; i < 4; i++) {
      try {
        _dmaChannels.emplace_back(_devicePath, i);
      }
      catch(const runtime_error& e) {
        break;
      }
    }
    _hasActiveException = false;
  }

  void XdmaBackend::close() {
    _ctrlIntf.reset();
    _dmaChannels.clear();
  }

  bool XdmaBackend::isOpen() { return !!_ctrlIntf; }

  bool XdmaBackend::isFunctional() const {
    if(!_ctrlIntf || _hasActiveException) {
      return false;
    }
    // TODO implement me
    return true;
  }

  XdmaIntfAbstract* XdmaBackend::_intfFromBar(uint8_t bar) {
    if(bar == 0) {
      return _ctrlIntf ? dynamic_cast<XdmaIntfAbstract*>(&_ctrlIntf.value()) : nullptr;
    }
    // 13 is magic value for DMA channel (by convention)
    // We provide N DMA channels starting from there
    const ssize_t dmaChIdx = bar - 13;
    if(dmaChIdx < 0 || dmaChIdx >= static_cast<ssize_t>(_dmaChannels.size())) {
      auto exc_string =
          std::string{"Couldn't find XDMA channel for BAR value "} + std::to_string(static_cast<int>(bar));
      throw ChimeraTK::logic_error(exc_string);
    }
    return dynamic_cast<XdmaIntfAbstract*>(&_dmaChannels[dmaChIdx]);
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

  void XdmaBackend::read(uint8_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
#ifdef _DEBUG
    std::cout << "read " << sizeInBytes << " bytes @ BAR" << bar << ", 0x" << std::hex << address << std::endl;
#endif
    auto intf = _intfFromBar(bar);
    intf->read(address, data, sizeInBytes);
#ifdef _DEBUGDUMP
    dump(data, sizeInBytes);
#endif
  }

  void XdmaBackend::write(uint8_t bar, uint64_t address, const int32_t* data, size_t sizeInBytes) {
#ifdef _DEBUG
    std::cout << "write " << sizeInBytes << " bytes @ BAR" << bar << ", 0x" << std::hex << address << std::endl;
#endif
    auto intf = _intfFromBar(bar);
    intf->write(address, data, sizeInBytes);
#ifdef _DEBUGDUMP
    dump(data, sizeInBytes);
#endif
  }

  std::string XdmaBackend::readDeviceInfo() {
    // TODO implement me
    return "";
  }

  boost::shared_ptr<DeviceBackend> XdmaBackend::createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    if(address.empty()) {
      throw ChimeraTK::logic_error("XDMA device address not specified.");
    }

    return boost::make_shared<XdmaBackend>("/dev/" + address, parameters["map"]);
  }

  void XdmaBackend::setException() { _hasActiveException = true; }

} // namespace ChimeraTK
