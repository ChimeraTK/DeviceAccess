// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomain.h"
#include "AsyncNDRegisterAccessor.h"
#include "VersionNumber.h"

#include <functional>

namespace ChimeraTK {

  template<typename DistributorType, typename BackendDataType>
  class AsyncDomainImpl : public AsyncDomain {
   public:
    explicit AsyncDomainImpl(
        std::function<boost::shared_ptr<DistributorType>(boost::shared_ptr<AsyncDomain>)> creatorFunction)
    : _creatorFunction(creatorFunction) {}

    /**
     * Distribute the data via the associated distribution tree.
     *
     * If the backend can determine a version number from the data, it has to do this before calling distribute and give
     * the version as an argument. Otherwise a new version is created under the domain lock.
     *
     * As the asynchronous subscription with its thread has to be started before activate() is called, it can happen
     * that distribute with newer data and a newer version number is called before activate is called with the initial
     * value. In this case, the data is stored and no data is distributed. The data will later be distributed during
     * activation instead of the older polled initial value. The return value is VersionNumber{nullptr}.
     *
     * In case distribute is called after activate, with a version number older than the polled initial value, the data
     * is dropped and not distributed. The return value is VersionNumber{nullptr}.
     *
     * @ return The version number that has been used for distribution, or VersionNumber{nullptr} if there was no distribution.
     */
    VersionNumber distribute(BackendDataType data, VersionNumber version = VersionNumber{nullptr});

    /**
     * Activate and distribute the initial value.
     *
     * If the backend can determine a version number from the data, it has to do this before calling distribute and give
     * the version as an argument. Otherwise a new version is created under the domain lock.
     *
     * In case distribute has been called before with a version number newer than the version of the polled initial
     * value, these data and version number are distributed instead.
     *
     *  @ return The version number that has been used for distribution.
     */
    VersionNumber activate(BackendDataType data, VersionNumber version = VersionNumber{nullptr});
    void deactivate();
    void sendException(const std::exception_ptr& e) noexcept override;

    template<typename UserDataType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

   protected:
    // Everything in this class is protected by the mutex from the AsyncDomain base class.
    boost::weak_ptr<DistributorType> _distributor;

    std::function<boost::shared_ptr<DistributorType>(boost::shared_ptr<AsyncDomain>)> _creatorFunction;

    // Data to resolve a race condition (see distribute and activate)
    BackendDataType _notDistributedData;
    VersionNumber _notDistributedVersion{nullptr};
    VersionNumber _activationVersion{nullptr};
  };

  /********************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  VersionNumber AsyncDomainImpl<DistributorType, BackendDataType>::distribute(
      BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);
    // everything incl. potential creation of a new version number must happen under the lock
    if(version == VersionNumber(nullptr)) {
      version = {};
    }

    if(!_isActive) {
      // Store the data. We might need it later if the data in activate is older due to a race condition.
      _notDistributedData = data;
      _notDistributedVersion = version;
      return VersionNumber{nullptr};
    }

    if(version < _activationVersion) {
      return VersionNumber{nullptr};
    }

    auto distributor = _distributor.lock();
    if(!distributor) {
      return VersionNumber{nullptr};
    }

    distributor->distribute(data, version);
    return version;
  }

  /********************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  VersionNumber AsyncDomainImpl<DistributorType, BackendDataType>::activate(
      BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);
    // everything incl. potential creation of a new version number must happen under the lock
    if(version == VersionNumber(nullptr)) {
      version = {};
    }

    _isActive = true;

    auto distributor = _distributor.lock();
    if(!distributor) {
      return VersionNumber{nullptr};
    }

    if(version >= _notDistributedVersion) {
      distributor->activate(data, version);
      _activationVersion = version;
    }
    else {
      // Due to a race condition, it has been tried to distribute newer data before activate was called.
      distributor->activate(_notDistributedData, _notDistributedVersion);
      _activationVersion = _notDistributedVersion;
    }
    return _activationVersion;
  }

  /********************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  void AsyncDomainImpl<DistributorType, BackendDataType>::deactivate() {
    std::lock_guard l(_mutex);

    _isActive = false;
  }

  /********************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  void AsyncDomainImpl<DistributorType, BackendDataType>::sendException(const std::exception_ptr& e) noexcept {
    std::lock_guard l(_mutex);

    if(!_isActive) {
      // don't send exceptions if async read is off
      return;
    }

    _isActive = false;
    auto distributor = _distributor.lock();
    if(!distributor) {
      return;
    }

    distributor->sendException(e);
  }

  /********************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  template<typename UserDataType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> AsyncDomainImpl<DistributorType, BackendDataType>::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard l(_mutex);

    auto distributor = _distributor.lock();
    if(!distributor) {
      distributor = _creatorFunction(shared_from_this());
      _distributor = distributor;
    }

    return distributor->template subscribe<UserDataType>(name, numberOfWords, wordOffsetInRegister, flags);
  }

} // namespace ChimeraTK
