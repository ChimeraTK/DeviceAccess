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
    explicit SubArrayAccessorDecorator(const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath path,
        size_t nElements, size_t elementOffset, const AccessModeFlags& flags);

    void doPreRead(TransferType type) override;
    void doPostRead(TransferType type, bool hasNewData) override;
    void doPreWrite(TransferType type, VersionNumber versionNumber) override;
    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

   protected:
    size_t _elementOffset;

    detail::ReferenceCountedUniqueLock _lock;

    VersionNumber _temporaryVersion; // set in preWrite and used in postWrite

    static boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> getTarget(
        const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath targetRegisterPath,
        const AccessModeFlags& flags);
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  SubArrayAccessorDecorator<UserType, TargetUserType>::SubArrayAccessorDecorator(
      const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath path, size_t nElements, size_t elementOffset,
      const AccessModeFlags& flags)
  : NDRegisterAccessorDecorator<UserType, TargetUserType>(getTarget(targetBackend, path, flags)),
    _elementOffset(elementOffset) {
    if(nElements == 0) {
      throw ChimeraTK::logic_error("SubArrayAccessorDecorator: nElements must not be 0 in " + this->getName());
    }

    if((nElements + elementOffset) > _target->getNumberOfSamples()) {
      throw ChimeraTK::logic_error("Requested offset + nElemements exceeds register size in " + this->getName());
    }

    for(auto& channel : buffer_2D) {
      channel.resize(nElements);
    }

    auto& sharedAccessorMultiTypeMap = detail::SharedAccessorMap::getInstance();

    path.setAltSeparator(".");
    detail::SharedAccessorKey key(targetBackend.get(), path);

    auto l = std::lock_guard(sharedAccessorMultiTypeMap._mutex);
    auto& map = boost::fusion::at_key<TargetUserType>(sharedAccessorMultiTypeMap._allTypesMap.table);

    auto& sharedAccessor = map[key];
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

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    _lock.lock();

    if(!_target->isWriteable()) {
      throw ChimeraTK::logic_error("Register \"" + TransferElement::getName() + "\" is not writeable.");
    }

    // When in a transfer group, only the first accessor to write to the _target can call read() in its preWrite()
    // Otherwise it will overwrite the
    if(_target->isReadable() && (!TransferElement::_isInTransferGroup || _lock.useCount() == 1)) {
      _target->read();
    }

    for(size_t i = 0; i < buffer_2D[0].size(); ++i) {
      _target->accessData(i + _elementOffset) = userTypeToUserType<TargetUserType, UserType>(buffer_2D[0][i]);
    }

    _temporaryVersion = std::max(versionNumber, _target->getVersionNumber());
    _target->setDataValidity(this->_dataValidity);
    _target->preWrite(type, _temporaryVersion);
  }

  /******************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPostWrite(
      TransferType type, [[maybe_unused]] VersionNumber versionNumber) {
    auto unlock = cppext::finally([this] { this->_lock.unlock(); });
    _target->postWrite(type, _temporaryVersion);
  }

  template<typename UserType, typename TargetUserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> SubArrayAccessorDecorator<UserType,
      TargetUserType>::getTarget(const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath targetRegisterPath,
      const AccessModeFlags& flags) {
    auto& sharedAccessorMultiTypeMap = detail::SharedAccessorMap::getInstance();

    targetRegisterPath.setAltSeparator(".");
    detail::SharedAccessorKey key(targetBackend.get(), targetRegisterPath);

    auto l = std::lock_guard(sharedAccessorMultiTypeMap._mutex);
    auto& map = boost::fusion::at_key<TargetUserType>(sharedAccessorMultiTypeMap._allTypesMap.table);

    auto& sharedAccessor = map[key];
    auto sharedInstance = sharedAccessor.accessor.lock();
    if(!sharedInstance) {
      auto newInstance = targetBackend->getRegisterAccessor<TargetUserType>(targetRegisterPath, 0, 0, flags);
      sharedAccessor.accessor = newInstance;
      return newInstance;
    }
    return sharedInstance;
  }

} // namespace ChimeraTK::detail
