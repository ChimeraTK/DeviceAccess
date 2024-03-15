// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncAccessorManager.h"
#include "InterruptControllerHandler.h"
#include "TransferGroup.h"

#include <memory>

namespace ChimeraTK {
  /**
   *  The TriggeredPollDistributor has std::nullptr_t source data type and is polling the data for the AsyncVariables
   *  via synchronous accessors in TransferGroup.
   *
   */
  class TriggeredPollDistributor : public SourceTypedAsyncAccessorManager<std::nullptr_t> {
   public:
    TriggeredPollDistributor(boost::shared_ptr<DeviceBackend> backend, boost::shared_ptr<TriggerDistributor> parent,
        boost::shared_ptr<AsyncDomain> asyncDomain);

    /** Poll all sync variables. */
    bool prepareIntermediateBuffers() override;

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(AccessorInstanceDescriptor const& descriptor);

   protected:
    TransferGroup _transferGroup;
    boost::shared_ptr<TriggerDistributor> _parent;
  };

  /********************************************************************************************************************/

  /** Implementation of the PolledAsyncVariable for the concrete UserType.
   */
  template<typename UserType>
  struct PolledAsyncVariable : public AsyncVariableImpl<UserType> {
    void fillSendBuffer() final;

    /** The constructor takes an already created synchronous accessor and a reference to the version variable.
     */
    explicit PolledAsyncVariable(boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, VersionNumber& v);

    boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor;
    VersionNumber& _version;

    unsigned int getNumberOfChannels() override { return syncAccessor->getNumberOfChannels(); }
    unsigned int getNumberOfSamples() override { return syncAccessor->getNumberOfSamples(); }
    const std::string& getUnit() override { return syncAccessor->getUnit(); }
    const std::string& getDescription() override { return syncAccessor->getDescription(); }
  };

  /********************************************************************************************************************/
  // Implementations
  /********************************************************************************************************************/

  template<typename UserType>
  std::unique_ptr<AsyncVariable> TriggeredPollDistributor::createAsyncVariable(
      AccessorInstanceDescriptor const& descriptor) {
    auto synchronousFlags = descriptor.flags;
    synchronousFlags.remove(AccessMode::wait_for_new_data);
    // Don't call backend->getSyncRegisterAccessor() here. It might skip the overriding of a backend.
    auto syncAccessor = _backend->getRegisterAccessor<UserType>(
        descriptor.name, descriptor.numberOfWords, descriptor.wordOffsetInRegister, synchronousFlags);
    // read the initial value before adding it to the transfer group
    if(_asyncDomain->unsafeGetIsActive()) {
      try {
        syncAccessor->read();
      }
      catch(ChimeraTK::runtime_error&) {
        // Nothing to do here. The backend's setException() has already been called by the syncAccessor.
      }
    }

    _transferGroup.addAccessor(syncAccessor);
    return std::make_unique<PolledAsyncVariable<UserType>>(syncAccessor, _version);
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void PolledAsyncVariable<UserType>::fillSendBuffer() {
    this->_sendBuffer.versionNumber = _version;
    this->_sendBuffer.dataValidity = syncAccessor->dataValidity();
    this->_sendBuffer.value.swap(syncAccessor->accessChannels());
  }

  /********************************************************************************************************************/
  template<typename UserType>
  PolledAsyncVariable<UserType>::PolledAsyncVariable(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, VersionNumber& v)
  : AsyncVariableImpl<UserType>(syncAccessor_->getNumberOfChannels(), syncAccessor_->getNumberOfSamples()),
    syncAccessor(syncAccessor_), _version(v) {}

} // namespace ChimeraTK
