#pragma once

#include <atomic>
#include <boost/core/noncopyable.hpp>

#include "NumericAddressedBackend.h"

namespace ChimeraTK {

class XdmaBackend : public NumericAddressedBackend, private boost::noncopyable {
private:
    int _deviceFd{0};
    std::string _deviceFilePath;
    std::atomic<bool> _hasActiveException{false};

    std::string _strerror(const std::string& msg) const;

public:
    explicit XdmaBackend(std::string deviceFilePath, std::string mapFileName = "");
    ~XdmaBackend();

    void open() override;
    void close() override;

    bool isFunctional() const override;

    void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address,
        std::map<std::string, std::string> parameters
    );

    void setException() override;
};

}
