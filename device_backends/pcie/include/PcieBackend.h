#ifndef CHIMERA_TK_PCIE_BACKEND_H
#define CHIMERA_TK_PCIE_BACKEND_H

#include <boost/function.hpp>
#include <stdint.h>
#include <stdlib.h>

#include "NumericAddressedBackend.h"

namespace ChimeraTK {

  /** A class to provide the Pcie device functionality."
   *
   */
  class PcieBackend : public NumericAddressedBackend {
   private:
    int _deviceID;
    unsigned long _ioctlPhysicalSlot;
    unsigned long _ioctlDriverVersion;
    unsigned long _ioctlDMA;
    std::string _deviceNodeName;

    std::atomic<bool> _hasActiveException{false};

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

    /** constructor called through createInstance to create device object */

   public:
    PcieBackend(std::string deviceNodeName, std::string mapFileName = "");
    ~PcieBackend() override;

    void open() override;
    void close() override;

    bool isFunctional() const override;

    void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;

    /*Host or parameters (at least for now) are just place holders as pcidevice
     * does not use them*/
    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    void setException() override { _hasActiveException = true; }
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_PCIE_BACKEND_H */
