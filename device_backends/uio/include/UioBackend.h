// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedBackend.h"

namespace ChimeraTK {

  class UioBackend : public NumericAddressedBackend {
   private:
    int _deviceID;
    void* _deviceUserBase;
    void* _deviceKernelBase;
    size_t _deviceMemSize;
    std::string _deviceNodeName;

    // Refactor to separate class ??
    void UioMMap();
    void UioUnmap();

    /* data */
   public:
    UioBackend(std::string deviceName, std::string mapFileName);
    ~UioBackend();

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    void open() override;
    void closeImpl() override;

    bool isFunctional() const override;

    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;
    uint64_t readUint64HexFromFile(std::string fileName);
  };

} // namespace ChimeraTK