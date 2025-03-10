// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../VersionNumber.h"
#include "AsyncNDRegisterAccessor.h"
#include "Domain.h"
#include "SubDomain.h"

#include <functional>

namespace ChimeraTK::async {

  template<typename BackendDataType>
  class DomainImpl : public Domain {
   public:
    DomainImpl(boost::shared_ptr<DeviceBackend> backend, size_t domainId) : _backend(backend), _id(domainId) {}

    /**
     * Distribute the data via the associated distribution tree.
     *
     * If the backend can determine a version number from the data, it has to do this before calling distribute and give
     * the version as an argument. Otherwise a new version is created under the domain lock.
     *
     * As the asynchronous subscription with its thread has to be started before activate() is called, it can happen
     * that distribute with newer data and a newer version number is called before activate is called with the initial
     * value. In this case, the data is stored and no data is distributed. The data will later be distributed during
     * activation instead of the older polled initial value. If data is stored for delayed distribution, the return
     * value is VersionNumber{nullptr}.
     *
     * In case distribute() is called after activate(), with a version number older than the polled initial value, the
     * data is dropped and not distributed. In this case the return value is VersionNumber{nullptr}.
     *
     * @ return The version number that has been used for distribution, or VersionNumber{nullptr} if there was no
     * distribution.
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

    void deactivate() override;
    void sendException(const std::exception_ptr& e) noexcept override;

    template<typename UserDataType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

   protected:
    // Everything in this class is protected by the mutex from the Domain base class.
    boost::weak_ptr<SubDomain<BackendDataType>> _subDomain;

    boost::shared_ptr<DeviceBackend> _backend;
    size_t _id;

    // Data to resolve a race condition (see distribute and activate)
    BackendDataType _notDistributedData;
    VersionNumber _notDistributedVersion{nullptr};
    VersionNumber _activationVersion{nullptr};
  };

  /********************************************************************************************************************/

  template<typename BackendDataType>
  VersionNumber DomainImpl<BackendDataType>::distribute(BackendDataType data, VersionNumber version) {
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

    auto subDomain = _subDomain.lock();
    if(!subDomain) {
      return VersionNumber{nullptr};
    }

    subDomain->distribute(data, version);
    return version;
  }

  /********************************************************************************************************************/

  template<typename BackendDataType>
  VersionNumber DomainImpl<BackendDataType>::activate(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);
    // everything incl. potential creation of a new version number must happen under the lock
    if(_isActive) {
      return VersionNumber(nullptr);
    }

    if(version == VersionNumber(nullptr)) {
      version = {};
    }

    _isActive = true;

    auto subDomain = _subDomain.lock();
    if(!subDomain) {
      return VersionNumber{nullptr};
    }

    if(version >= _notDistributedVersion) {
      subDomain->activate(data, version);
      _activationVersion = version;
    }
    else {
      // Due to a race condition, it has been tried to distribute newer data before activate was called.
      subDomain->activate(_notDistributedData, _notDistributedVersion);
      _activationVersion = _notDistributedVersion;
    }
    return _activationVersion;
  }

  /********************************************************************************************************************/

  template<typename BackendDataType>
  void DomainImpl<BackendDataType>::deactivate() {
    std::lock_guard l(_mutex);

    _isActive = false;
  }

  /********************************************************************************************************************/

  template<typename BackendDataType>
  void DomainImpl<BackendDataType>::sendException(const std::exception_ptr& e) noexcept {
    std::lock_guard l(_mutex);

    if(!_isActive) {
      // don't send exceptions if async read is off
      return;
    }

    _isActive = false;
    auto subDomain = _subDomain.lock();
    if(!subDomain) {
      return;
    }

    subDomain->sendException(e);
  }

  /********************************************************************************************************************/

  template<typename BackendDataType>
  template<typename UserDataType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserDataType>> DomainImpl<BackendDataType>::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard l(_mutex);

    auto subDomain = _subDomain.lock();
    if(!subDomain) {
      subDomain = boost::make_shared<SubDomain<BackendDataType>>(
          _backend, std::vector<size_t>({_id}), nullptr, shared_from_this());
      _subDomain = subDomain;
    }

    return subDomain->template subscribe<UserDataType>(name, numberOfWords, wordOffsetInRegister, flags);
  }

} // namespace ChimeraTK::async
