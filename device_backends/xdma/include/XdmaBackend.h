#pragma once

#include <atomic>
#include <optional>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "CtrlIntf.h"
#include "DmaIntf.h"

#include "NumericAddressedBackend.h"

namespace ChimeraTK {

  class XdmaBackend : public NumericAddressedBackend, private boost::noncopyable {
   private:
    std::optional<CtrlIntf> _ctrlIntf;
    std::vector<DmaIntf> _dmaChannels;

    const std::string _devicePath;

    XdmaIntfAbstract* _intfFromBar(uint64_t bar);

   public:
    explicit XdmaBackend(std::string devicePath, std::string mapFileName = "");
    ~XdmaBackend() override;

    void open() override;
    void closeImpl() override;
    bool isOpen() override;

    bool isFunctional() const override;

    void dump(const int32_t* data, size_t nbytes);
    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, const int32_t* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);
  };

} // namespace ChimeraTK
