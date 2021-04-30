#include "XdmaBackend.h"

#include <boost/make_shared.hpp>


namespace ChimeraTK {

XdmaBackend::XdmaBackend(std::string devicePath, std::string mapFileName)
:NumericAddressedBackend(mapFileName)
,_devicePath(devicePath)
{
}

XdmaBackend::~XdmaBackend()
{
}

void XdmaBackend::open()
{
#ifdef _DEBUG
    std::cout << "open xdma dev: " << _devicePath << std::endl;
#endif
    if (_ctrlIntf) {
        if (isFunctional()) {
            return;
        }
        close();
    }
    // TODO: retrieve mmap_(offs,size) from map file
    _ctrlIntf.emplace(_devicePath);

    // Build vector of max. 4 DMA channels
    _dmaChannels.clear();
    for (size_t i = 0; i < 4; i++) {
        try {
            _dmaChannels.emplace_back(_devicePath, i);
        }
        catch (const runtime_error& e) {
            break;
        }
    }
    _hasActiveException = false;
}

void XdmaBackend::close()
{
    _ctrlIntf.reset();
    _dmaChannels.clear();
}

bool XdmaBackend::isOpen() {
    return !!_ctrlIntf;
}

bool XdmaBackend::isFunctional() const {
    if (!_ctrlIntf || _hasActiveException) {
        return false;
    }
    // TODO implement me
    return true;
}

XdmaIntfAbstract* XdmaBackend::_intfFromBar(uint8_t bar)
{
    if (bar == 0) {
        return _ctrlIntf ? dynamic_cast<XdmaIntfAbstract*>(&_ctrlIntf.value()) : nullptr;
    }
    const size_t dmaChIdx = bar - 1;
    if (dmaChIdx >= _dmaChannels.size()) {
        return nullptr;
    }
    return dynamic_cast<XdmaIntfAbstract*>(&_dmaChannels[dmaChIdx]);
}

void XdmaBackend::read(uint8_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
#ifdef _DEBUG
    std::cout << "read " << sizeInBytes << " bytes @ BAR" << bar << ", 0x" << std::hex << address << std::endl;
#endif
    auto intf = _intfFromBar(bar);
    if (!intf) {
        // TODO: error handling
        return;
    }
    intf->read(address, data, sizeInBytes);
}

void XdmaBackend::write(uint8_t bar, uint64_t address, const int32_t* data, size_t sizeInBytes) {
#ifdef _DEBUG
    std::cout << "write " << sizeInBytes << " bytes @ BAR" << bar << ", 0x" << std::hex << address << std::endl;
#endif
    auto intf = _intfFromBar(bar);
    if (!intf) {
        // TODO: error handling
        return;
    }
    intf->write(address, data, sizeInBytes);
}

std::string XdmaBackend::readDeviceInfo() {
    // TODO implement me
    return "";
}

boost::shared_ptr<DeviceBackend> XdmaBackend::createInstance(
    std::string address,
    std::map<std::string, std::string> parameters) {

    if(address.empty()) {
      throw ChimeraTK::logic_error("XDMA device address not specified.");
    }

    return boost::make_shared<XdmaBackend>("/dev/" + address, parameters["map"]);
}

void XdmaBackend::setException() {
    _hasActiveException = true;
}

}
