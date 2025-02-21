// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../ScalarRegisterAccessor.h"
#include "../TransferGroup.h"
#include "AsyncAccessorManager.h"
#include "DataConsistencyRealm.h"
#include "MuxedInterruptDistributor.h"

#include <memory>

namespace ChimeraTK::async {
  /**
   *  The TriggeredPollDistributor has std::nullptr_t source data type and is polling the data for the AsyncVariables
   *  via synchronous accessors in TransferGroup.
   *
   */
  class TriggeredPollDistributor : public SourceTypedAsyncAccessorManager<std::nullptr_t> {
   public:
    TriggeredPollDistributor(boost::shared_ptr<DeviceBackend> backend,
        boost::shared_ptr<SubDomain<std::nullptr_t>> parent, boost::shared_ptr<Domain> asyncDomain);

    /** Poll all sync variables. */
    bool prepareIntermediateBuffers() override;

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(AccessorInstanceDescriptor const& descriptor);

    VersionNumber getVersion() const { return _version; }

    bool getForceFaulty() const { return _forceFaulty; }

   protected:
    TransferGroup _transferGroup;
    boost::shared_ptr<SubDomain<std::nullptr_t>> _parent;
    std::shared_ptr<DataConsistencyRealm> _dataConsistencyRealm;
    ScalarRegisterAccessor<DataConsistencyKey::BaseType> _dataConsistencyKeyAccessor;
    bool _forceFaulty{false};
    VersionNumber _lastVersion{nullptr};
  };

  /********************************************************************************************************************/

  /** Implementation of the PolledAsyncVariable for the concrete UserType.
   */
  template<typename UserType>
  struct PolledAsyncVariable : public AsyncVariableImpl<UserType> {
    void fillSendBuffer() final;

    /// The constructor takes an already created synchronous accessor and a reference to the owing distributor
    explicit PolledAsyncVariable(
        boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, TriggeredPollDistributor& owner);

    unsigned int getNumberOfChannels() override { return _syncAccessor->getNumberOfChannels(); }
    unsigned int getNumberOfSamples() override { return _syncAccessor->getNumberOfSamples(); }
    const std::string& getUnit() override { return _syncAccessor->getUnit(); }
    const std::string& getDescription() override { return _syncAccessor->getDescription(); }

   protected:
    boost::shared_ptr<NDRegisterAccessor<UserType>> _syncAccessor;

    TriggeredPollDistributor& _owner;
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
    return std::make_unique<PolledAsyncVariable<UserType>>(syncAccessor, *this);
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void PolledAsyncVariable<UserType>::fillSendBuffer() {
    this->_sendBuffer.versionNumber = _owner.getVersion();
    this->_sendBuffer.dataValidity = !_owner.getForceFaulty() ? _syncAccessor->dataValidity() : DataValidity::faulty;
    this->_sendBuffer.value.swap(_syncAccessor->accessChannels());
  }

  /********************************************************************************************************************/
  template<typename UserType>
  PolledAsyncVariable<UserType>::PolledAsyncVariable(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, TriggeredPollDistributor& owner)
  : AsyncVariableImpl<UserType>(syncAccessor_->getNumberOfChannels(), syncAccessor_->getNumberOfSamples()),
    _syncAccessor(syncAccessor_), _owner(owner) {}

} // namespace ChimeraTK::async
