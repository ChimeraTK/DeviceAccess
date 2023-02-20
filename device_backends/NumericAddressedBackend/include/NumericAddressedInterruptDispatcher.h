// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncAccessorManager.h"
#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "TransferGroup.h"

#include <memory>
#include <mutex>

namespace ChimeraTK {
  /** Typeless base class. The implementations will have a list of all asynchronous
   *  accessors and one synchronous accessor.
   */
  struct NumericAddressedAsyncVariable {
    virtual ~NumericAddressedAsyncVariable() = default;

    /** Fill the user buffer from the sync accessor, and replace the version number with the given version.
     */
    virtual void fillSendBuffer(VersionNumber const& version) = 0;
  };

  /** The NumericAddressedInterruptDispatcher has two main functionalities:
   *  * It calls functions for all asynchronous accessors associated with one interrupt
   *  * It serves as a subscription manager
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class has implements a lock so
   *  dispatching an interrupt is safe against concurrent subscription/unsubscription.
   */
  class NumericAddressedInterruptDispatcher : public AsyncAccessorManager {
   public:
    /** Trigger all NumericAddressedAsyncVariables that are stored in this dispatcher. Creates a new VersionNumber and
     * sends all data with this version.
     */
    VersionNumber trigger();

    NumericAddressedInterruptDispatcher();

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(
        const boost::shared_ptr<DeviceBackend>& backend, AccessorInstanceDescriptor const& descriptor, bool isActive);

    // bool prepareActivate(VersionNumber const& v) override;
    VersionNumber activate() override;

   protected:
    void asyncVariableMapChanged() override {
      if(_asyncVariables.empty()) {
        // all asyncVariables have been unsubscribed - we can finally remove the TransferGroup
        // This is important since it's elements still keep shared pointers to the backend, creating a shared-ptr loop
        // replace it by a new TransferGroup just in case another async variable would be created later
        _transferGroup = std::make_unique<TransferGroup>();
      }
    }
    // unique_ptr because we want to delete it manually
    std::unique_ptr<TransferGroup> _transferGroup{new TransferGroup};
  };

  /** Implementation of the NumericAddressedAsyncVariable for the concrete UserType.
   */
  template<typename UserType>
  struct NumericAddressedAsyncVariableImpl : public AsyncVariableImpl<UserType>, public NumericAddressedAsyncVariable {
    void fillSendBuffer(VersionNumber const& version) final;

    /** The constructor takes an already created synchronous accessor and a flag
     *  whether the variable is active. If the variable is active all new subscribers will automatically
     *  be activated and immediately get their initial value.
     */
    explicit NumericAddressedAsyncVariableImpl(boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_);

    boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor;

    unsigned int getNumberOfChannels() override { return syncAccessor->getNumberOfChannels(); }
    unsigned int getNumberOfSamples() override { return syncAccessor->getNumberOfSamples(); }
    const std::string& getUnit() override { return syncAccessor->getUnit(); }
    const std::string& getDescription() override { return syncAccessor->getDescription(); }
    bool isWriteable() override { return syncAccessor->isWriteable(); }
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  template<typename UserType>
  std::unique_ptr<AsyncVariable> NumericAddressedInterruptDispatcher::createAsyncVariable(
      const boost::shared_ptr<DeviceBackend>& backend, AccessorInstanceDescriptor const& descriptor, bool isActive) {
    auto synchronousFlags = descriptor.flags;
    synchronousFlags.remove(AccessMode::wait_for_new_data);
    // Don't call backend->getSyncRegisterAccessor() here. It might skip the overriding of a backend.
    auto syncAccessor = backend->getRegisterAccessor<UserType>(
        descriptor.name, descriptor.numberOfWords, descriptor.wordOffsetInRegister, synchronousFlags);
    // read the initial value before adding it to the transfer group
    if(isActive) {
      try {
        syncAccessor->read();
      }
      catch(ChimeraTK::runtime_error&) {
        isActive = false;
      }
    }

    _transferGroup->addAccessor(syncAccessor);
    return std::make_unique<NumericAddressedAsyncVariableImpl<UserType>>(syncAccessor);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::fillSendBuffer(VersionNumber const& version) {
    this->_sendBuffer.versionNumber = version;
    this->_sendBuffer.dataValidity = syncAccessor->dataValidity();
    this->_sendBuffer.value.swap(syncAccessor->accessChannels());
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  NumericAddressedAsyncVariableImpl<UserType>::NumericAddressedAsyncVariableImpl(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_)
  : AsyncVariableImpl<UserType>(syncAccessor_->getNumberOfChannels(), syncAccessor_->getNumberOfSamples()),
    syncAccessor(syncAccessor_) {
    NumericAddressedAsyncVariableImpl<UserType>::fillSendBuffer({});
  }

} // namespace ChimeraTK
