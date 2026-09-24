// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../ScalarRegisterAccessor.h"
#include "../SelectorGate.h"
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

    /**
     * Build the wake-level gate for a subscription to a register declared 'selectedBy', or an empty
     * gate if the register is not conditionally active. Defined in the .cc so that
     * NumericAddressedBackend is a complete type (avoids a header include cycle).
     */
    SelectorGate buildSelectorGate(const AccessorInstanceDescriptor& descriptor);

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
  bool fillSendBuffer() final;

    /// The constructor takes an already created synchronous accessor and a reference to the owing distributor
    explicit PolledAsyncVariable(boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_,
        TriggeredPollDistributor& owner, SelectorGate selectorGate = SelectorGate());

    unsigned int getNumberOfChannels() override { return _syncAccessor->getNumberOfChannels(); }
    unsigned int getNumberOfSamples() override { return _syncAccessor->getNumberOfSamples(); }
    const std::string& getUnit() override { return _syncAccessor->getUnit(); }
    const std::string& getDescription() override { return _syncAccessor->getDescription(); }

   protected:
    boost::shared_ptr<NDRegisterAccessor<UserType>> _syncAccessor;

    TriggeredPollDistributor& _owner;

    /// Optional wake-level gate: while the selection is not met the subscription's delivery is suppressed entirely
    /// (after the initial value has been delivered), so wait_for_new_data consumers do not wake on the inactive
    /// alternative.
    SelectorGate _selectorGate;

    /// Whether the initial value has been delivered yet. The initial delivery always happens (a consumer activated
    /// while the selection is not met still receives its initial faulty value); only subsequent distributions are
    /// suppressed while unselected.
    bool _initialDelivered{false};

    /// Version number of the most recent *published* (selected) delivery; used to keep the version
    /// unchanged while unselected so consumers stay pending.
    VersionNumber _lastPublishedVersion{nullptr};
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

    // Wake-level gate for subscriptions to a register declared 'selectedBy' (see buildSelectorGate).
    // The synchronous accessor already gates validity in doPostRead (validity level); this gate
    // additionally suppresses wake/version advance while unselected.
    SelectorGate selectorGate = buildSelectorGate(descriptor);

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
    return std::make_unique<PolledAsyncVariable<UserType>>(syncAccessor, *this, std::move(selectorGate));
  }

  /********************************************************************************************************************/
  template<typename UserType>
  bool PolledAsyncVariable<UserType>::fillSendBuffer() {
    // Wake/version level gate. The initial value is always delivered (a consumer activated while the selection is
    // not met still receives its initial faulty value). Once the initial value has been delivered, subsequent
    // distributions while the selection is not met are suppressed entirely (return false), so wait_for_new_data
    // consumers do not wake with the inactive alternative and `_lastPublishedVersion` stays untouched. Once
    // selected, deliver with the domain version and the accessor's (already gated) validity.
    bool unselected = false;
    if(_selectorGate) {
      _selectorGate.check();
      unselected = (_selectorGate.dataValidity() == DataValidity::faulty);
    }
    if(unselected && _initialDelivered) {
      return false;
    }
    _initialDelivered = true;
    this->_sendBuffer.versionNumber = _owner.getVersion();
    this->_sendBuffer.dataValidity = !_owner.getForceFaulty() ? _syncAccessor->dataValidity() : DataValidity::faulty;
    _lastPublishedVersion = this->_sendBuffer.versionNumber;
    this->_sendBuffer.value.swap(_syncAccessor->accessChannels());
    return true;
  }

  /********************************************************************************************************************/
  template<typename UserType>
  PolledAsyncVariable<UserType>::PolledAsyncVariable(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, TriggeredPollDistributor& owner,
      SelectorGate selectorGate)
  : AsyncVariableImpl<UserType>(syncAccessor_->getNumberOfChannels(), syncAccessor_->getNumberOfSamples()),
    _syncAccessor(syncAccessor_), _owner(owner), _selectorGate(std::move(selectorGate)) {}

} // namespace ChimeraTK::async
