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
  class NumericAddressedInterruptDispatcher;

  /** Base class for address-based device backends (e.g. PICe, Rebot, ...) */
  class NumericAddressedBackend : public DeviceBackendImpl {
   public:
    NumericAddressedBackend(std::string mapFileName = "");

    ~NumericAddressedBackend() override {}

    /* interface using 32bit address for backwards compatibility */
    virtual void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);
    virtual void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes);

    virtual void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes);
    virtual void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes);

    virtual bool barIndexValid(uint64_t bar);

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

    //    void activateAsyncRead() noexcept final;
    //    void setException() final;
    void activateAsyncRead() noexcept override;
    void setException() override;

    /** Deactivates all asynchronous accessors and calls closeImpl().
     */
    void close() final;

    /** All backends derrived from NumericAddressedBackend must implement closeImpl() instead of close. Like this it
     *  is assured that the deactivation of the asynchronous accessors is always executed.
     */
    virtual void closeImpl() {}

    /** This function is called every time an accessor which is assicated with the particular interupt controller and interrupt number
     *  is created. The idea is to have a lazy initialisation of the interrupt handling threads, so only those threads are running for which
     *  accessors have been created. The function implementation must check whether the according thread is already running and should do nothing
     *  when called a second time.
     *
     *  The function has an empty default implementation.
     */
    virtual void startInterruptHandlingThread(unsigned int interruptControllerNumber, unsigned int interruptNumber);

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

    // internal helper function to get the a synchronous accessor, which is also needed by the asynchronous version internally, but is not given out
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getSyncRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    std::atomic<bool> _hasActiveException{false};

    friend NumericAddressedLowLevelTransferElement;
    friend NumericAddressedInterruptDispatcher;

    /** Function to be called by implementing backend when an interrupt arrives. It usually is
     *  called from the interrupt handling thread.
     *
     *  Throws std::out_of_range if an invalid interruptControllerNumber/interruptNumber is given as parameter.
     */
    void dispatchInterrupt(int interruptControllerNumber, int interruptNumber);

   private:
    /** This variable is private so the map cannot be altered by derriving backends. The only thing the backends have to
     *  do is trigger an interrupt, and this is done through dispatchInterrupt() which makes sure that the map is not modified.
     */
    std::map<std::pair<int, int>, boost::shared_ptr<NumericAddressedInterruptDispatcher>> _interruptDispatchers;
  };

} // namespace ChimeraTK

#endif /*CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H*/
