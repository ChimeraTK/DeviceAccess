// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedBackend.h"

#include <boost/function.hpp>

#include <cstdint>
#include <cstdlib>

namespace ChimeraTK {

  /** A class to provide the Pcie device functionality."
   *
   */
  class PcieBackend : public NumericAddressedBackend {
   private:
    int _deviceID;
    uint64_t _ioctlPhysicalSlot;
    uint64_t _ioctlDriverVersion;
    uint64_t _ioctlDMA;
    std::string _deviceNodeName;

    /// A function pointer which calls the correct dma read function (via ioctl or
    /// via struct)
    boost::function<void(uint8_t bar, uint32_t address, int32_t* data, size_t size)> _readDMAFunction;

    /// A function pointer which call the right write function
    // boost::function< void (uint8_t, uint32_t, int32_t const *) >
    // _writeFunction;

    /// For the area we need something with a loop for the struct write.
    /// For the direct write this is the same as writeFunction.
    boost::function<void(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes)> _writeFunction;

    boost::function<void(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes)> _readFunction;

    void readDMAViaIoctl(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);
    void readDMAViaStruct(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);

    std::string createErrorStringWithErrnoText(std::string const& startText);
    void determineDriverAndConfigureIoctl();
    void writeInternal(uint8_t bar, uint32_t address, int32_t const* data);
    void writeWithStruct(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes);
    /** This function is the same for one or multiple words */
    void directWrite(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes);

    void readInternal(uint8_t bar, uint32_t address, int32_t* data);
    void readWithStruct(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);
    /** This function is the same for one or multiple words */
    void directRead(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);

    size_t minimumTransferAlignment([[maybe_unused]] uint64_t bar) const override { return 4; }

    bool checkConnection() const;

    /** constructor called through createInstance to create device object */

   public:
    explicit PcieBackend(std::string deviceNodeName, const std::string& mapFileName = "");
    ~PcieBackend() override;

    void open() override;
    void closeImpl() override;

    void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;

    /*Host or parameters (at least for now) are just place holders as pcidevice
     * does not use them*/
    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);
  };

} // namespace ChimeraTK
