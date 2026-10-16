// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../ClosedGateQuery.h"
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

    /** The version on open of the backend the distributor polls. Used by the PolledAsyncVariable to tell an initial
     * value (after (re)activation) apart from a regular update. */
    VersionNumber getVersionOnOpen() const { return _backend->getVersionOnOpen(); }

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

    /** Suppresses the delivery through a closed gate, unless the value is the mandatory initial value after a
     * (re)activation. The gate result is read through the ClosedGateQuery the polled accessor brings in. */
    void send() final;

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

    /** The gate query of the polled accessor, or null if the accessor is not gated. Resolved once at construction. */
    ClosedGateQuery* _gateQuery{nullptr};

    /** The version number of the last value that was actually sent. Compared with the version on open to recognise the
     * mandatory initial value after a (re)activation. */
    VersionNumber _lastSentVersion{nullptr};
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
    _syncAccessor(syncAccessor_), _owner(owner) {
    _gateQuery = dynamic_cast<ClosedGateQuery*>(_syncAccessor.get());
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void PolledAsyncVariable<UserType>::send() {
    // The mandatory initial value after a (re)activation is produced without a gate evaluation and must be delivered
    // even through a closed gate. It is recognised by the version falling behind the version on open.
    const bool isInitialValue = _lastSentVersion < _owner.getVersionOnOpen();
    if(!isInitialValue && _gateQuery != nullptr && _gateQuery->isGateClosed()) {
      return;
    }
    AsyncVariableImpl<UserType>::send();
    _lastSentVersion = this->_sendBuffer.versionNumber;
  }

} // namespace ChimeraTK::async
