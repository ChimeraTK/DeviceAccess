// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CtrlIntf.h"
#include "DmaIntf.h"
#include "EventFile.h"
#include "NumericAddressedBackend.h"

#include <boost/core/noncopyable.hpp>

#include <atomic>
#include <optional>
#include <vector>

namespace ChimeraTK {

  class XdmaBackend : public NumericAddressedBackend, private boost::noncopyable {
   private:
    static constexpr size_t _maxDmaChannels = 4;
    static constexpr size_t _maxInterrupts = 16;

    std::optional<CtrlIntf> _ctrlIntf;
    std::vector<DmaIntf> _dmaChannels;
    std::array<std::unique_ptr<EventFile>, _maxInterrupts> _eventFiles;
    std::array<bool, _maxInterrupts> _startInterruptHandlingCalled{};

    const std::string _devicePath;

    XdmaIntfAbstract& _intfFromBar(uint64_t bar);

   public:
    explicit XdmaBackend(std::string devicePath, std::string mapFileName = "");
    ~XdmaBackend() override;

    void open() override;
    void closeImpl() override;
    bool isOpen() override;

    void dump(const int32_t* data, size_t nbytes);
    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, const int32_t* data, size_t sizeInBytes) override;
    void startInterruptHandlingThread(uint32_t interruptNumber) override;
    using NumericAddressedBackend::dispatchInterrupt; // make public for EventThread

    std::string readDeviceInfo() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);
  };

} // namespace ChimeraTK
