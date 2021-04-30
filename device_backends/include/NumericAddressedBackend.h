#ifndef CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H
#define CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H

#include <fcntl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>

#include "DeviceBackendImpl.h"

namespace ChimeraTK {

  class NumericAddressedLowLevelTransferElement;

  /** Base class for address-based device backends (e.g. PICe, Rebot, ...) */
  class NumericAddressedBackend : public DeviceBackendImpl {
   public:
    NumericAddressedBackend(std::string mapFileName = "");

    virtual ~NumericAddressedBackend() {}

    virtual void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) = 0;

    virtual void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) = 0;

    virtual std::string readDeviceInfo() = 0;

    /**
     * @brief Determines whether the backend supports merging of requests (read or
     * write)
     *
     * Should return true if the backend supports that several consecutive write
     * or read operations are merged into one single read or write request. If a
     * deriving backend cannot handle such requests, it can prevent this by
     * returning false here.
     *
     * @return true if supported, false otherwise
     */
    virtual bool canMergeRequests() const { return true; }

    /**
     * @brief Determines the supported minimum alignment for any read/write requests.
     *
     * If the backend expects a particular alignment for read()/write() calls it should return a value biggern than 1.
     * The address and sizeInBytes arguments of the read()/write() calls will be always an integer multiple of this
     * number. Any unaligned transfers will be changed to meet these criteria (additional padding data will be thrown
     * away).
     *
     * The default implementation returns 1, which means no special alignment is required.
     *
     * @return Minimum alignment in bytes
     */
    virtual size_t minimumTransferAlignment() const { return 1; }

    boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const;

    boost::shared_ptr<RegisterInfoMap::RegisterInfo> getRegisterInfo(const RegisterPath& registerPathName);

   protected:
    /// resolve register name to address with error checks
    void checkRegister(const std::string& regName, const std::string& regModule, size_t dataSize, uint32_t addRegOffset,
        uint32_t& retDataSize, uint32_t& retRegOff, uint8_t& retRegBar) const;

    /// map from register names to addresses
    boost::shared_ptr<RegisterInfoMap> _registerMap;

    /// mutex for protecting unaligned access
    std::mutex _unalignedAccess;

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    friend NumericAddressedLowLevelTransferElement;
  };

} // namespace ChimeraTK

#endif /*CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H*/
