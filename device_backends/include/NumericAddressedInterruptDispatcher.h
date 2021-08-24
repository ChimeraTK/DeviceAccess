#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "AsyncAccessorManager.h"
#include <mutex>

namespace ChimeraTK {

  /** Typeless base class. The implementations will have a list of all asynchronous
   *  accessors and one synchrounous accessor.
   */
  struct NumericAddressedAsyncVariable {
    virtual ~NumericAddressedAsyncVariable() = default;

    /** Read the synchronous accessor and push the data to all subscribers,
     *  using the specified version number.
     */
    virtual void trigger(VersionNumber const& version) = 0;
  };

  /** The NumericAddressedInterruptDispatcher has two main functionalities:
   *  * It calls functions for all asynchronous accessors associated with one interrupt
   *  * It serves as a subscription manager
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class has implements a lock so
   *  dispatching an interrupt is safe against concurrent subscriptions/unsibscriptions.
   */
  class NumericAddressedInterruptDispatcher : public AsyncAccessorManager {
   public:
    /** Trigger all NumericAddressedAsyncVariables that are stored in this dispatcher. Creates a new VersionNumber and sends
     *  all data with this version.
     */
    VersionNumber trigger();

    NumericAddressedInterruptDispatcher();

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(
        boost::shared_ptr<DeviceBackend> backend, AccessorInstanceDescriptor const& descriptor, bool isActive);
  };

  /** Implementation of the NumericAddressedAsyncVariable for the concrete UserType.
   */
  template<typename UserType>
  struct NumericAddressedAsyncVariableImpl : public AsyncVariableImpl<UserType>, public NumericAddressedAsyncVariable {
    void trigger(VersionNumber const& version) override;

    /** The constructor takes an already created synchronous accessor and a flag
     *  whether the variable is active. If the variable is active all new subscribers will automatically
     *  be activated and immediately get their initial value.
     */
    NumericAddressedAsyncVariableImpl(boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, bool isActive);

    boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor;

    unsigned int getNumberOfChannels() override { return syncAccessor->getNumberOfChannels(); }
    unsigned int getNumberOfSamples() override { return syncAccessor->getNumberOfSamples(); }
    const std::string& getUnit() override { return syncAccessor->getUnit(); }
    const std::string& getDescription() override { return syncAccessor->getDescription(); }
    bool isWriteable() override { return syncAccessor->isWriteable(); }

    typename NDRegisterAccessor<UserType>::Buffer getInitialValue(VersionNumber const& versionNumber) override;
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::trigger(VersionNumber const& version) {
    try {
      syncAccessor->read();
      this->executeWithCopy(
          [](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager,
                  AsyncAccessorManager::AccessorInstanceDescriptor>>& accessor,
              typename NDRegisterAccessor<UserType>::Buffer& buf) { accessor->sendDestructively(buf); },
          syncAccessor->accessChannels(), version, syncAccessor->dataValidity());
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the sync accessor
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  std::unique_ptr<AsyncVariable> NumericAddressedInterruptDispatcher::createAsyncVariable(
      boost::shared_ptr<DeviceBackend> backend, AccessorInstanceDescriptor const& descriptor, bool isActive) {
    auto synchronousFlags = descriptor.flags;
    synchronousFlags.remove(AccessMode::wait_for_new_data);
    // Don't call backend->getSyncRegisterAccessor() here. It might skip the overriding of a backend.
    auto syncAccessor = backend->getRegisterAccessor<UserType>(
        descriptor.name, descriptor.numberOfWords, descriptor.wordOffsetInRegister, synchronousFlags);
    return std::make_unique<NumericAddressedAsyncVariableImpl<UserType>>(syncAccessor, isActive);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  NumericAddressedAsyncVariableImpl<UserType>::NumericAddressedAsyncVariableImpl(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, bool isActive)
  : AsyncVariableImpl<UserType>(isActive, syncAccessor_->getNumberOfChannels(), syncAccessor_->getNumberOfSamples()),
    syncAccessor(syncAccessor_) {}

  template<typename UserType>
  typename NDRegisterAccessor<UserType>::Buffer NumericAddressedAsyncVariableImpl<UserType>::getInitialValue(
      VersionNumber const& versionNumber) {
    typename NDRegisterAccessor<UserType>::Buffer b{syncAccessor->getNumberOfChannels(),
        syncAccessor->getNumberOfSamples()}; // fixme:: Try an implementation without allocating buffer
    try {
      syncAccessor->read();
      b.value = syncAccessor->accessChannels(); // hew, this is expensive. Copying a full buffer_2D
      b.dataValidity = syncAccessor->dataValidity();
      b.versionNumber = versionNumber;
    }
    catch(ChimeraTK::runtime_error&) {
      // no action needed. The synchronous read() already triggered backend->setException();
    }
    return b;
  }

  //  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedAsyncVariableImpl);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor);

} // namespace ChimeraTK
