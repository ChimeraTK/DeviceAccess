// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CountedRecursiveMutex.h"
#include "NDRegisterAccessorDecorator.h"
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
    ~SubArrayAccessorDecorator();

    void doPreRead(TransferType type) override;
    void doPostRead(TransferType type, bool hasNewData) override;
    void doPreWrite(TransferType type, VersionNumber versionNumber) override;
    void doPostWrite(TransferType type, VersionNumber versionNumber) override;
    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override;

    [[nodiscard]] bool isWriteable() const override;
    [[nodiscard]] bool isReadOnly() const override;

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> newElement) override;

   protected:
    size_t _elementOffset;
    bool _isWriteable; // if in a transfer group with overlapping sub arrays of other accessor we turn to read only mode

    boost::shared_ptr<DeviceBackend> _targetBackend;

    std::shared_ptr<SharedAccessors::TargetSharedState>
        _targetSharedState; // we must hold an instance of the shared state to safely hold a lock to the mutex within,
                            // and a pointer to the shared buffer
    std::unique_lock<CountedRecursiveMutex> _lock;
    NDRegisterAccessor<TargetUserType>::Buffer* _sharedBuffer{nullptr};

    VersionNumber _temporaryVersion; // set in preWrite and used in postWrite

    static boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> getTarget(
        const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath targetRegisterPath,
        const AccessModeFlags& flags);

    /// swap the data content and copy the data validity from the shared buffer to the target accessor
    void sharedBufferToTarget();

    /// swap the data content and copy data validity and version from the target accessor to the shared buffer
    void targetToSharedBuffer();
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  SubArrayAccessorDecorator<UserType, TargetUserType>::SubArrayAccessorDecorator(
      const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath path, size_t nElements, size_t elementOffset,
      const AccessModeFlags& flags)
  : NDRegisterAccessorDecorator<UserType, TargetUserType>(getTarget(targetBackend, path, flags)),
    _elementOffset(elementOffset), _isWriteable(_target->isWriteable()), _targetBackend(targetBackend) {
    if(nElements == 0) {
      throw ChimeraTK::logic_error("SubArrayAccessorDecorator: nElements must not be 0 in " + this->getName());
    }

    if((nElements + elementOffset) > _target->getNumberOfSamples()) {
      throw ChimeraTK::logic_error("Requested offset + nElemements exceeds register size in " + this->getName());
    }

    for(auto& channel : buffer_2D) {
      channel.resize(nElements);
    }

    auto& sharedAccessors = detail::SharedAccessors::getInstance();

    path.setAltSeparator(".");
    detail::SharedAccessorKey key(targetBackend.get(), path);

    _targetSharedState = sharedAccessors.getTargetSharedState<TargetUserType>(key);
    // The buffer in the shared state is a variant. It has to be of TargetUserType
    _sharedBuffer =
        std::get_if<SharedAccessors::TargetSharedState::UserBuffer<TargetUserType>>(&(_targetSharedState->dataBuffer));
    assert(_sharedBuffer); // getTargetSharedState throws if the type does not match, so get_if() should always work
    _lock = std::unique_lock<detail::CountedRecursiveMutex>(_targetSharedState->mutex, std::defer_lock);
  }
  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  SubArrayAccessorDecorator<UserType, TargetUserType>::~SubArrayAccessorDecorator() {
    auto& sharedAccessorMutexes = detail::SharedAccessors::getInstance();
    sharedAccessorMutexes.removeTransferElement(_target->getId());
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPreRead(TransferType type) {
    _lock.lock();

    // In a transfer group: only swap the shared buffer to the target once, before calling the target's preRead().
    // We do it in the first call to preRead(). It does not matter because we do nothing else here.
    if(_lock.mutex()->useCount() == 1) {
      sharedBufferToTarget();
      _target->preRead(type);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPostRead(
      [[maybe_unused]] TransferType type, bool hasNewData) {
    auto unlock = cppext::finally([this] { this->_lock.unlock(); });

    // step 1: target postRead() and swap the data into the shared state
    auto& sharedAccessors = detail::SharedAccessors::getInstance();
    // the use count is counting down, so the first call has the full number
    if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
      // whether the postRead throws or we have new data: The target buffer must be swapped back to keep the shared
      // state intact
      auto swapBack = cppext::finally([this] { targetToSharedBuffer(); });
      _target->postRead(type, hasNewData);
    } // the swapBack finally will be executed with this clothing brace

    if(!hasNewData) {
      return;
    }

    // Step 2: Fill the user buffer (which is only a part of the shared buffer)
    for(size_t i = 0; i < buffer_2D[0].size(); ++i) {
      buffer_2D[0][i] = userTypeToUserType<UserType, TargetUserType>(_sharedBuffer->value[0][i + _elementOffset]);
    }
    this->_versionNumber = std::max(this->_versionNumber, _sharedBuffer->versionNumber);
    this->_dataValidity = _sharedBuffer->dataValidity;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    _lock.lock();

    if(!_isWriteable) {
      throw ChimeraTK::logic_error("Register \"" + TransferElement::getName() + "\" is not writeable.");
    }

    // Read-Remember-Modify-Write:
    // Only read to the shared state once after opening the device. The content there must not change while the software
    // is connected. Reading always would be a performance penalty and subject to race condition. If the
    // content can really change on the device, the software business logic has to implement the read/modify/write and
    // be aware of the race conditions. It intentionally is not handles in the framework logic.
    if(_target->isReadable() && (_sharedBuffer->versionNumber < _targetBackend->getVersionOnOpen())) {
      sharedBufferToTarget();
      auto swapBack =
          cppext::finally([this] { targetToSharedBuffer(); }); // read might throw, then we must get our buffer back
      _target->read();
    } // the swapBack finally will be executed with this clothing brace

    for(size_t i = 0; i < buffer_2D[0].size(); ++i) {
      _sharedBuffer->value[0][i + _elementOffset] = userTypeToUserType<TargetUserType, UserType>(buffer_2D[0][i]);
    }
    _sharedBuffer->versionNumber = std::max(versionNumber, _sharedBuffer->versionNumber);
    if((_sharedBuffer->dataValidity == DataValidity::ok) // don't overwrite faulty from previous preWrite()
                                                         // in this transfer
        || (_lock.mutex()->useCount() == 1)) {           // always set for first preWrite()
      _sharedBuffer->dataValidity = this->_dataValidity;
    }

    // According to spec, only the first call to preWrite() does something, so it has to be the last accessor
    // in a transfer group. (use count is always one if not in transfer group)
    auto& sharedAccessors = detail::SharedAccessors::getInstance();
    if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
      _temporaryVersion = std::max(_sharedBuffer->versionNumber, _target->getVersionNumber());
      // It is safe to swap the shared buffer into the target. doWriteTransferDestructively() is
      // overridden in this decorator to always call the non-destructive write transfer on the target, so we can
      // swap the intact shared state back into the shared buffer in doPostWrite().
      sharedBufferToTarget();
      _target->preWrite(type, _temporaryVersion);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::doPostWrite(
      TransferType type, [[maybe_unused]] VersionNumber versionNumber) {
    auto unlock = cppext::finally([this] { this->_lock.unlock(); });

    auto& sharedAccessors = detail::SharedAccessors::getInstance();
    if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
      // even if postWrite throws: The target buffer must be swapped back to keep the shared state intact
      auto swapBack = cppext::finally([this] { targetToSharedBuffer(); });
      _target->postWrite(type, _temporaryVersion);
    } // the swapBack finally will be executed with this clothing brace
  }

  template<typename UserType, typename TargetUserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> SubArrayAccessorDecorator<UserType,
      TargetUserType>::getTarget(const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath targetRegisterPath,
      const AccessModeFlags& flags) {
    auto newAccessor = targetBackend->getRegisterAccessor<TargetUserType>(targetRegisterPath, 0, 0, flags);
    auto& sharedAccessors = detail::SharedAccessors::getInstance();
    sharedAccessors.addTransferElement(newAccessor->getId());

    return newAccessor;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::replaceTransferElement(
      boost::shared_ptr<ChimeraTK::TransferElement> newElement) {
    auto castedToThisType =
        boost::dynamic_pointer_cast<SubArrayAccessorDecorator<UserType, TargetUserType>>(newElement);
    if(castedToThisType && castedToThisType.get() != this && castedToThisType->_target->mayReplaceOther(_target)) {
      // Check for overlap in the two ranges. Turn of writing if overlapping.there is no overlap
      if((castedToThisType->_elementOffset < (_elementOffset + this->getNumberOfSamples())) &&
          (_elementOffset < (castedToThisType->_elementOffset + castedToThisType->getNumberOfSamples()))) {
        if(!this->isReadable()) {
          throw ChimeraTK::logic_error("Cannot turn off writing for write-only accessors. You can't put overlapping "
                                       "write-only accessors into a transfer group.");
        }
        castedToThisType->_isWriteable = false;
        _isWriteable = false;
      }
    }

    auto castedToTargetType = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<TargetUserType>>(newElement);
    if(castedToTargetType && newElement->mayReplaceOther(_target)) {
      if(_target != newElement) {
        // No copy decorator.
        auto oldTarget = _target->getId();
        _target = castedToTargetType;
        // Bookkeeping: combine the original target and the replaced target's use count
        auto& sharedAccessorMutexes = detail::SharedAccessors::getInstance();
        sharedAccessorMutexes.combineTransferSharedStates(oldTarget, _target->getId());
      }
    }
    else {
      _target->replaceTransferElement(newElement);
    }
    _target->setExceptionBackend(this->_exceptionBackend);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::sharedBufferToTarget() {
    for(size_t chan = 0; chan < _target->getNumberOfChannels(); ++chan) {
      _sharedBuffer->value[chan].swap(_target->accessChannel(chan));
    }
    _target->setDataValidity(_sharedBuffer->dataValidity);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void SubArrayAccessorDecorator<UserType, TargetUserType>::targetToSharedBuffer() {
    for(size_t chan = 0; chan < _target->getNumberOfChannels(); ++chan) {
      _sharedBuffer->value[chan].swap(_target->accessChannel(chan));
    }
    _sharedBuffer->versionNumber = std::max(_sharedBuffer->versionNumber, _target->getVersionNumber());
    _sharedBuffer->dataValidity = _target->dataValidity();
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  bool SubArrayAccessorDecorator<UserType, TargetUserType>::doWriteTransferDestructively(
      ChimeraTK::VersionNumber versionNumber) {
    // Always call the non-destructive write. We must be able to swap back the intact buffer into the _sharedState.
    // It is better to do the copy here instead of always copying in preWrite(), which cannot know whether
    // writeTransfer() or writeTransferDestructively() will be called next. In case of writeTransfer(), or if the
    // backend does not have a destructive optimisation (which usually is the case for hardware accessing backends),
    // copying in preWrite() would be an extra copy.
    return this->_target->writeTransfer(versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  bool SubArrayAccessorDecorator<UserType, TargetUserType>::isWriteable() const {
    return _isWriteable;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  bool SubArrayAccessorDecorator<UserType, TargetUserType>::isReadOnly() const {
    // If the accessor is not writeable it has to be readable, and hence is read only.
    return !_isWriteable;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
