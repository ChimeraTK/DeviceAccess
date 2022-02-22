#pragma once

#include "NumericAddressedRegisterCatalogue.h"
#include "DeviceBackendImpl.h"
#include "VersionNumber.h"

#include <string>
#include <mutex>

namespace ChimeraTK {

  class NumericAddressedLowLevelTransferElement;
  class NumericAddressedInterruptDispatcher;

  /** Base class for address-based device backends (e.g. PICe, Rebot, ...) */
  class NumericAddressedBackend : public DeviceBackendImpl {
   public:
    NumericAddressedBackend(std::string mapFileName = "");

    ~NumericAddressedBackend() override {}

    /**
     * Read function to be implemented by backends.
     * 
     * TODO: Add documentation!
     */
    virtual void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes);

    /**
     * Write function to be implemented by backends.
     * 
     * TODO: Add documentation!
     */
    virtual void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes);

    /**
     * Deprecated read function using 32bit address for backwards compatibility. Old backends which have not yet
     * been updated to the new 64 bit address interface will implement this. Please implement the read() function
     * with the 64 bit address signature instead!
     */
    virtual void read([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address, [[maybe_unused]] int32_t* data,
        [[maybe_unused]] size_t sizeInBytes);

    /**
     * Deprecated write function using 32bit address for backwards compatibility. Old backends which have not yet
     * been updated to the new 64 bit address interface will implement this. Please implement the write() function
     * with the 64 bit address signature instead!
     */
    virtual void write([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
        [[maybe_unused]] int32_t const* data, [[maybe_unused]] size_t sizeInBytes);

    /**
     * Function to be implemented by the backends. Returns whether the given bar number is valid.
     */
    virtual bool barIndexValid(uint64_t bar);

    /**
     * @brief Determines whether the backend supports merging of requests (read or write)
     *
     * Should return true if the backend supports that several consecutive write or read operations are merged into one
     * single read or write request. If a deriving backend cannot handle such requests, it can prevent this by returning
     * false here.
     *
     * @return true if supported, false otherwise
     */
    virtual bool canMergeRequests() const { return true; }

    /**
     * @brief Determines the supported minimum alignment for any read/write requests.
     *
     * If the backend expects a particular alignment for read()/write() calls it should return a value bigger than 1.
     * The address and sizeInBytes arguments of the read()/write() calls will be always an integer multiple of this
     * number. Any unaligned transfers will be changed to meet these criteria (additional padding data will be thrown
     * away).
     *
     * The default implementation returns 1, which means no special alignment is required.
     *
     * @return Minimum alignment in bytes
     */
    virtual size_t minimumTransferAlignment([[maybe_unused]] uint64_t bar) const { return 1; }

    RegisterCatalogue getRegisterCatalogue() const override;

    MetadataCatalogue getMetadataCatalogue() const override;

    /**
     * @brief getRegisterInfo returns a NumericAddressedRegisterInfo object for the given register. This is mainly used
     * by accessor implementations
     * 
     * @param registerPathName identifies which register to return the RegisterInfo for.
     * 
     * @return Shared pointer to the NumericAddressedRegisterInfo object in the catalogue.
     */
    NumericAddressedRegisterInfo getRegisterInfo(const RegisterPath& registerPathName);

    void activateAsyncRead() noexcept override;
    void setException() override;

    /** 
     *  Deactivates all asynchronous accessors and calls closeImpl().
     */
    void close() final;

    /** 
     *  All backends derrived from NumericAddressedBackend must implement closeImpl() instead of close. Like this it
     *  is assured that the deactivation of the asynchronous accessors is always executed.
     */
    virtual void closeImpl() {}

    /** 
     *  This function is called every time an accessor which is assicated with the particular interupt controller and
     *  interrupt number is created. The idea is to have a lazy initialisation of the interrupt handling threads, so
     *  only those threads are running for which accessors have been created. The function implementation must check
     *  whether the according thread is already running and should do nothing when called a second time.
     *
     *  The function has an empty default implementation.
     */
    virtual void startInterruptHandlingThread(unsigned int interruptControllerNumber, unsigned int interruptNumber);

   protected:
    /// resolve register name to address with error checks
    void checkRegister(const std::string& regName, const std::string& regModule, size_t dataSize, uint32_t addRegOffset,
        uint32_t& retDataSize, uint32_t& retRegOff, uint8_t& retRegBar) const;

    /// register catalogue
    NumericAddressedRegisterCatalogue _registerMap;

    /// metadata catalogue
    MetadataCatalogue _metadataCatalogue;

    /// mutex for protecting unaligned access
    std::mutex _unalignedAccess;

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    // internal helper function to get the a synchronous accessor, which is also needed by the asynchronous version internally, but is not given out
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getSyncRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    std::atomic<bool> _hasActiveException{false};

    friend NumericAddressedLowLevelTransferElement;
    friend NumericAddressedInterruptDispatcher;

    /** 
     *  Function to be called by implementing backend when an interrupt arrives. It usually is
     *  called from the interrupt handling thread.
     *
     *  Throws std::out_of_range if an invalid interruptControllerNumber/interruptNumber is given as parameter.
     *
     *   @returns The version number that was send with all data in this interrupt.
     */
    VersionNumber dispatchInterrupt(int interruptControllerNumber, int interruptNumber);

   private:
    /** 
     *  This variable is private so the map cannot be altered by derriving backends. The only thing the backends have to
     *  do is trigger an interrupt, and this is done through dispatchInterrupt() which makes sure that the map is not modified.
     */
    std::map<std::pair<int, int>, boost::shared_ptr<NumericAddressedInterruptDispatcher>> _interruptDispatchers;
  };

} // namespace ChimeraTK
