// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedBackend.h"
#include "UioAccess.h"

#include <thread>

namespace ChimeraTK {

  class UioBackend : public NumericAddressedBackend {
   private:
    std::shared_ptr<UioAccess> _uioAccess;

    std::mutex _launchThreadMutex;
    std::thread _interruptWaitingThread;
    std::atomic<bool> _stopInterruptLoop{false}; // Used to shut down thread

    void waitForInterruptLoop();

    /* data */
   public:
    UioBackend(std::string deviceName, std::string mapFileName);
    ~UioBackend() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    void open() override;
    void closeImpl() override;

    bool barIndexValid(uint64_t bar) override;

    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override;
    void startInterruptHandlingThread(uint32_t interruptNumber) override;

    std::string readDeviceInfo() override;
  };

} // namespace ChimeraTK
