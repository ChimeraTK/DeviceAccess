#include "XdmaBackend.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK {

XdmaBackend::XdmaBackend(std::string deviceFilePath, std::string mapFileName)
:NumericAddressedBackend(mapFileName)
,_deviceFilePath(deviceFilePath)
{
}

XdmaBackend::~XdmaBackend()
{
}

void XdmaBackend::open()
{
#ifdef _DEBUG
    std::cout << "open xdma dev: " << _deviceFilePath << std::endl;
#endif
    if (!!_deviceFd) {
        if (isFunctional()) {
            return;
        }
        close();
    }
    _deviceFd = ::open(_deviceFilePath.c_str(), O_RDWR);
    if(_deviceFd < 0) {
        throw ChimeraTK::runtime_error(_strerror("Cannot open device: "));
    }
    _hasActiveException = false;
}

void XdmaBackend::close()
{
    if (!!_deviceFd) {
        ::close(_deviceFd);
    }
    _deviceFd = 0;
}

bool XdmaBackend::isFunctional() const {
    if (!_deviceFd || _hasActiveException) {
        return false;
    }
    // TODO implement me
    return true;
}

void XdmaBackend::read([[maybe_unused]] uint8_t bar,
                       [[maybe_unused]] uint32_t address,
                       [[maybe_unused]] int32_t* data,
                       [[maybe_unused]] size_t sizeInBytes) {
    // TODO implement me
}

void XdmaBackend::write([[maybe_unused]] uint8_t bar,
                        [[maybe_unused]] uint32_t address,
                        [[maybe_unused]] int32_t const* data,
                        [[maybe_unused]] size_t sizeInBytes) {
    // TODO implement me
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

std::string XdmaBackend::_strerror(const std::string& msg) const {
    char tmp[255];
    return msg + _deviceFilePath + ": " + strerror_r(errno, tmp, sizeof(tmp));
}

}
