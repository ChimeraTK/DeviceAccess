// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DoubleBufferAccessorDecorator.h"
namespace ChimeraTK {

  template<typename UserType>
  DoubleBufferAccessorDecorator<UserType>::DoubleBufferAccessorDecorator(
      const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
      const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex)
  : NDRegisterAccessorDecorator<UserType>(target), _doubleBufferInfo(std::move(doubleBufferConfig)), _backend(backend),
    _mutex(std::move(mutex)), _transferLock(*_mutex, std::defer_lock) {
    _enableDoubleBufferReg =
        backend->getRegisterAccessor<uint32_t>(_doubleBufferInfo.enableRegisterPath, 1, _doubleBufferInfo.index, {});
    _currentBufferNumberReg = backend->getRegisterAccessor<uint32_t>(
        _doubleBufferInfo.inactiveBufferRegisterPath, 1, _doubleBufferInfo.index, {});

    auto numSamples = _target->getNumberOfSamples();
    auto accessFlags = _target->getAccessModeFlags();
    auto primaryName = _target->getName();
    auto buf0Name = primaryName + ".BUF0";
    auto buf1Name = primaryName + ".BUF1";

    _target = backend->getRegisterAccessor<UserType>(buf0Name, numSamples, 0, accessFlags);
    _secondBufferReg = backend->getRegisterAccessor<UserType>(buf1Name, numSamples, 0, accessFlags);

    {
      std::lock_guard<detail::CountedRecursiveMutex> lg(*_mutex);
      if(_mutex->useCount() == 1) {
        _enableDoubleBufferReg->accessChannel(0)[0] = 1;
        _enableDoubleBufferReg->write();
      }
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPreRead(TransferType type) {
    // Acquire lock for full transfer lifecycle
    _transferLock.lock(); // blocks other threads
    try {
      // acquire a lock in firmware (disable buffer swapping)
      _enableDoubleBufferReg->accessData(0) = 0;
      _enableDoubleBufferReg->write();

      // check which buffer is now in use by the firmware
      _currentBufferNumberReg->read();
      _currentBuffer = _currentBufferNumberReg->accessData(0);
      // if current buffer 1, it means firmware writes now to buffer1, so use target (buffer 0), else use
      // _secondBufferReg (buffer 1)
      if(_currentBuffer == 1) {
        _target->preRead(type);
      }
      else {
        _secondBufferReg->preRead(type);
      }
    }
    catch(...) {
      _transferLock.unlock(); // avoid deadlock on exception
      throw;
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doReadTransferSynchronously() {
    if(_currentBuffer == 1) {
      _target->readTransfer();
    }
    else {
      _secondBufferReg->readTransfer();
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    try {
      if(_currentBuffer == 1) {
        _target->postRead(type, hasNewData);
      }
      else {
        _secondBufferReg->postRead(type, hasNewData);
      }

      // acquire a lock in firmware (enable buffer swapping)
      _enableDoubleBufferReg->accessData(0) = 1;
      _enableDoubleBufferReg->write();
      // set version and data validity of this object
      this->_versionNumber = {};
      if(_currentBuffer == 1) {
        this->_dataValidity = _target->dataValidity();
      }
      else {
        this->_dataValidity = _secondBufferReg->dataValidity();
      }

      // Note: TransferElement Spec E.6.1 dictates that the version number and data validity needs to be set before this
      // check.
      if(!hasNewData) {
        return;
      }

      // Swap buffer_2D if new data
      if(hasNewData) {
        auto& reg = (_currentBuffer == 1) ? _target : _secondBufferReg;
        for(size_t i = 0; i < reg->getNumberOfChannels(); ++i) {
          ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(reg->accessChannel(i));
        }
      }
    }
    catch(...) {
      _transferLock.unlock();
      throw;
    }
    // Release lock last thing
    _transferLock.unlock();
  }

  template<typename UserType>
  std::vector<boost::shared_ptr<TransferElement>> DoubleBufferAccessorDecorator<
      UserType>::getHardwareAccessingElements() {
    // returning only this means the DoubleBufferAccessorDecorator will not be optimized when put into TransferGroup
    // optimizing would break our handshake protocol, since it reorders transfers
    return {TransferElement::shared_from_this()};
  }

  template<typename UserType>
  bool DoubleBufferAccessorDecorator<UserType>::mayReplaceOther(
      const boost::shared_ptr<const TransferElement>& other) const {
    auto otherDoubleBuffer = boost::dynamic_pointer_cast<const DoubleBufferAccessorDecorator<UserType>>(other);
    if(!otherDoubleBuffer) return false;
    if(_backend.get() != otherDoubleBuffer->_backend.get()) return false;
    if(_target->getName() != otherDoubleBuffer->_target->getName()) return false;
    // if(_target->getNumberOfSamples() != otherDoubleBuffer->_target->getNumberOfSamples()) return false;
    return true;
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DoubleBufferAccessorDecorator);

} // namespace ChimeraTK
