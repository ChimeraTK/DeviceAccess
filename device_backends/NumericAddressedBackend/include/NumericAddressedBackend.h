// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomainImpl.h"
#include "DeviceBackendImpl.h"
#include "InterruptControllerHandler.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <boost/pointer_cast.hpp>

#include <mutex>
#include <string>

namespace ChimeraTK {

  class NumericAddressedLowLevelTransferElement;
  class TriggeredPollDistributor;

  /** Base class for address-based device backends (e.g. PICe, Rebot, ...) */
  class NumericAddressedBackend : public DeviceBackendImpl {
   public:
    explicit NumericAddressedBackend(const std::string& mapFileName = "",
        std::unique_ptr<NumericAddressedRegisterCatalogue> registerMapPointer =
            std::make_unique<NumericAddressedRegisterCatalogue>());

    ~NumericAddressedBackend() override = default;

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
     *
     * Note: deprecated with warning since 2022-07-28 remove after end of 2023.
     */
    [[deprecated]] virtual void read([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
        [[maybe_unused]] int32_t* data, [[maybe_unused]] size_t sizeInBytes);

    /**
     * Deprecated write function using 32bit address for backwards compatibility. Old backends which have not yet
     * been updated to the new 64 bit address interface will implement this. Please implement the write() function
     * with the 64 bit address signature instead!
     *
     * Note: deprecated with warning since 2022-07-28, remove after end of 2023.
     */
    [[deprecated]] virtual void write([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
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
     *  Activate/create the subscription for a given interrupt (for instance by starting the according interrupt
     *  handling thread). A shared pointer to the AsyncDomain is handed as a parameter. The backend has to store
     *  it together with the subscription (usually as a weak pointer) and use it to distribute the interrupt.
     *
     *  A future is returned. It becomes ready when the subscription is actually active (e.g. from
     *  inside the an interrupt handling thread after the initialisation sequence with the hardware is done).
     *
     *  If the subscription already exists and is active, a ready future is returned.
     *
     *  The function has an empty default implementation which returns a ready future.
     */
    virtual std::future<void> activateSubscription(
        uint32_t interruptNumber, boost::shared_ptr<AsyncDomainImpl<std::nullptr_t>> asyncDomain);

    /** Turn off the internal variable which remembers that async is active. */
    void setExceptionImpl() noexcept override;

    /** Implementation of createInterruptControllerHandler which is accessing the _interruptControllerHandlerFactory. */
    boost::shared_ptr<InterruptControllerHandler> createInterruptControllerHandler(
        std::vector<uint32_t> const& controllerID,
        boost::shared_ptr<TriggerDistributor<std::nullptr_t>> parent) override;

    /**
     *  Get the initial value for a certain AsyncDomain. The return value is the backend specific user type (which can
     *  vary for different async domains) and the according version number.
     *  For the NumericAddressedBackend the BackendSpecificUserType is nullptr_t, which does not contain any information
     *  about the version number. Hence this implementation always returns [nullptr, VersionNumber{nullptr}].
     *  The code has to be template code because it is called with the template parameter from the AsyncDomainsContainer.
     */
    template<typename BackendSpecificUserType>
    std::pair<BackendSpecificUserType, VersionNumber> getAsyncDomainInitialValue(size_t asyncDomainId);

   protected:
    /*
     * Register catalogue. A reference is used here which is filled from _registerMapPointer in the constructor to allow
     * backend implementations to provide their own type based on the NumericAddressedRegisterCatalogue.
     */
    std::unique_ptr<NumericAddressedRegisterCatalogue> _registerMapPointer;
    NumericAddressedRegisterCatalogue& _registerMap;

    /// metadata catalogue
    MetadataCatalogue _metadataCatalogue;

    /// mutex for protecting unaligned access
    std::mutex _unalignedAccess;

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    friend NumericAddressedLowLevelTransferElement;
    friend TriggeredPollDistributor;

    template<class UserType, class ConverterType>
    friend class NumericAddressedBackendMuxedRegisterAccessor;

   private:
    InterruptControllerHandlerFactory _interruptControllerHandlerFactory{this};

    // internal helper function to get the a synchronous accessor, which is also needed by the asynchronous version
    // internally, but is not given out
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getSyncRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /** We have to remember this in case a new AsyncDomain is created after calling ActivateAsyncRead. */
    std::atomic_bool _asyncIsActive{false};
  };

  /********************************************************************************************************************/

  template<typename BackendSpecificUserType>
  std::pair<BackendSpecificUserType, VersionNumber> NumericAddressedBackend::getAsyncDomainInitialValue(
      [[maybe_unused]] size_t asyncDomainId) {
    static_assert(std::is_same<BackendSpecificUserType, std::nullptr_t>::value,
        "NumericAddressedBackend only supports AsyncDomain<nullptr_t>.");
    return {nullptr, VersionNumber{nullptr}};
  }

} // namespace ChimeraTK
