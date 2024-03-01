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

    void distribute(BackendDataType data, VersionNumber version);
    void activate(BackendDataType data, VersionNumber version);
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
  };

  //******************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  void AsyncDomainImpl<DistributorType, BackendDataType>::distribute(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);

    if(!_isActive) {
      // Store the data. We might need it later if the data in activate is older due to a race condition
      _notDistributedData = data;
      _notDistributedVersion = version;
      return;
    }

    auto distributor = _distributor.lock();
    if(!distributor) {
      return;
    }

    distributor->distribute(data, version);
  }

  //******************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  void AsyncDomainImpl<DistributorType, BackendDataType>::activate(BackendDataType data, VersionNumber version) {
    std::lock_guard l(_mutex);

    _isActive = true;

    auto distributor = _distributor.lock();
    if(!distributor) {
      return;
    }

    if(version >= _notDistributedVersion) {
      distributor->activate(data, version);
    }
    else {
      // due to a race condition, data has already been tried to
      distributor->activate(_notDistributedData, _notDistributedVersion);
    }
  }

  //******************************************************************************************************************/

  template<typename DistributorType, typename BackendDataType>
  void AsyncDomainImpl<DistributorType, BackendDataType>::deactivate() {
    std::lock_guard l(_mutex);

    _isActive = false;
  }

  //******************************************************************************************************************/

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

  //******************************************************************************************************************/

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
