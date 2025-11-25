// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorDecorator.h"
#include "ReferenceCountedUniqueLock.h"
#include "SharedAccessor.h"

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  /** Class to decorate a one-dimensional NDRegisterAccessor and only access a sub-range.
   * This is used by backends which always have to transfer the full array, and decorate it if only a sub-range or
   * scalar accessor is requested.
   */
  template<typename UserType, typename TargetUserType = UserType>
  class SubArrayAccessorDecorator : public NDRegisterAccessorDecorator<UserType, TargetUserType> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetUserType>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetUserType>::_target;

   public:
    explicit SubArrayAccessorDecorator(const boost::shared_ptr<DeviceBackend>& targetBackend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target, size_t nElements,
        size_t elementOffset);

    void doPreRead(TransferType type) override;
    void doPostRead(TransferType type, bool hasNewData) override;

   protected:
    size_t _elementOffset;

    detail::ReferenceCountedUniqueLock _lock;
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  SubArrayAccessorDecorator<UserType, TargetUserType>::SubArrayAccessorDecorator(
      const boost::shared_ptr<DeviceBackend>& targetBackend,
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target, size_t nElements,
      size_t elementOffset)
  : NDRegisterAccessorDecorator<UserType, TargetUserType>(target), _elementOffset(elementOffset) {
    if(nElements == 0) {
      nElements = target->getNumberOfSamples();
    }

    if((nElements + elementOffset) > target->getNumberOfSamples()) {
      throw ChimeraTK::logic_error("Requested offset + nElemements exceeds register size in " + this->getName());
    }

    for(auto& channel : buffer_2D) {
      channel.resize(nElements);
    }

    auto& sharedAccessorMultiTypeMap = detail::SharedAccessorMap::getInstance();

    RegisterPath path{target->getName()};
    path.setAltSeparator(".");
    detail::SharedAccessorKey key(targetBackend.get(), path);

    auto l = std::lock_guard(sharedAccessorMultiTypeMap._mutex);
    auto& map = boost::fusion::at_key<TargetUserType>(sharedAccessorMultiTypeMap._allTypesMap.table);

    auto& sharedAccessor = map[key];
    auto sharedInstance = sharedAccessor.accessor.lock();
    if(!sharedInstance) {
      sharedAccessor.accessor = target;
    }
    else {
      assert(sharedInstance == target);
    }
    _lock = detail::ReferenceCountedUniqueLock(sharedAccessor.mutex);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPreRead(TransferType type) {
    _lock.lock();

    _target->preRead(type);
  }

  /********************************************************************************************************************/
  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPostRead(
      [[maybe_unused]] TransferType type, bool hasNewData) {
    auto unlock = cppext::finally([this] { this->_lock.unlock(); });
    _target->postRead(type, hasNewData);

    if(!hasNewData) {
      return;
    }

    for(size_t i = 0; i < buffer_2D[0].size(); ++i) {
      buffer_2D[0][i] = userTypeToUserType<UserType, TargetUserType>(_target->accessData(i + _elementOffset));
    }
    this->_versionNumber = std::max(this->_versionNumber, _target->getVersionNumber());
    this->_dataValidity = _target->dataValidity();
  }

} // namespace ChimeraTK::detail
