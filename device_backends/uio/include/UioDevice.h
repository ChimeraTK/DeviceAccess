// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/filesystem.hpp>

#include <string>

namespace ChimeraTK {
  class UioDevice {
   private:
    boost::filesystem::path _deviceFilePath;
    int _deviceFileDescriptor = 0;
    void* _deviceUserBase = nullptr;
    void* _deviceKernelBase = nullptr;
    size_t _deviceMemSize = 0;

    void UioMMap();
    void UioUnmap();
    uint64_t readUint64HexFromFile(std::string fileName);

   public:
    UioDevice(std::string deviceFilePath);
    ~UioDevice();

    void open();
    void close();

    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes);
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes);

    std ::string getDeviceFilePath();
  };
} // namespace ChimeraTK
