// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DoubleBufferAccessor.h"

#include <utility>
namespace ChimeraTK {

  template<typename UserType>
  DoubleBufferAccessor<UserType>::DoubleBufferAccessor(
      NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
      const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex,
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags,
      const SelectedBy* selectedBy)
  : NDRegisterAccessor<UserType>(registerPathName, flags), _doubleBufferInfo(std::move(doubleBufferConfig)),
    _backend(backend), _mutex(std::move(mutex)), _transferLock(*_mutex, std::defer_lock) {
    _enableDoubleBufferReg =
        backend->getRegisterAccessor<uint32_t>(_doubleBufferInfo.enableRegisterPath, 1, _doubleBufferInfo.index, {});
    _currentBufferNumberReg = backend->getRegisterAccessor<uint32_t>(
        _doubleBufferInfo.inactiveBufferRegisterPath, 1, _doubleBufferInfo.index, {});

    // A register-level 'selectedBy' gates the whole 2D double buffer on a single selector. Mirror the selector
    // accessor creation of SelectedByDecorator so this low-level element can skip its own physical data read on the
    // interrupt path when unselected.
    if(selectedBy) {
      auto numericBackend = boost::static_pointer_cast<NumericAddressedBackend>(_backend);
      _selectorAccessor = boost::make_shared<ScalarRegisterAccessor<int64_t>>(
          numericBackend->template getSyncRegisterAccessor<int64_t>(selectedBy->regPath, 0, 0, {}));
      _expectedValue = selectedBy->val;
    }

    auto buf0Name = registerPathName + ".BUF0";
    auto buf1Name = registerPathName + ".BUF1";

    _buffer0 = backend->getRegisterAccessor<UserType>(buf0Name, numberOfWords, wordOffsetInRegister, flags);
    _buffer1 = backend->getRegisterAccessor<UserType>(buf1Name, numberOfWords, wordOffsetInRegister, flags);
    size_t nChannels = _buffer0->getNumberOfChannels();

    this->buffer_2D.resize(nChannels);
    for(size_t i = 0; i < nChannels; ++i) {
      buffer_2D[i].resize(numberOfWords);
    }

    {
      std::lock_guard<detail::CountedRecursiveMutex> lg(*_mutex);
      if(_mutex->useCount() == 1 && _backend->isOpen()) {
        _enableDoubleBufferReg->accessChannel(0)[0] = 1;
        _enableDoubleBufferReg->write();
      }
    }
  }

  template<typename UserType>
  void DoubleBufferAccessor<UserType>::doPreRead(TransferType type) {
    // Acquire lock for full transfer lifecycle
    _transferLock.lock(); // blocks other threads
    // acquire a lock in firmware (disable buffer swapping)
    if(_mutex->useCount() == 1) {
      _enableDoubleBufferReg->accessData(0) = 0;
      _enableDoubleBufferReg->write();
    }
    // check which buffer is now in use by the firmware
    _currentBufferNumberReg->read();
    _currentBuffer = _currentBufferNumberReg->accessData(0);
    // if current buffer 1, it means firmware writes now to buffer1, so use target (buffer 0), else use
    // _secondBufferReg (buffer 1)
    if(_currentBuffer == 1) {
      _buffer0->preRead(type);
    }
    else {
      _buffer1->preRead(type);
    }
  }

  template<typename UserType>
  void DoubleBufferAccessor<UserType>::doReadTransferSynchronously() {
    // On the interrupt path with skipping enabled, evaluate the selection gate before the transfer so the
    // (expensive, muxed) physical data read can be skipped entirely while unselected. The selector is still read so
    // a selection change is picked up on a later interrupt. The polled path (skipping disabled) always performs the
    // read; the gating validity is applied by the wrapping SelectedByDecorator's doPostRead().
    _transferSkipped = false;
    if(_selectorAccessor && _skipWhenUnselected) {
      _selectorAccessor->read();
      _lastGateOpen = (static_cast<int64_t>(*_selectorAccessor) == _expectedValue);
      if(!_lastGateOpen) {
        // Gate closed: skip the physical buffer read. E.6.1 requires the data validity to be set before the final
        // (not-new) check; mark the data faulty. doPostRead() reports it as not-new and does not swap the buffer.
        this->_dataValidity = DataValidity::faulty;
        _transferSkipped = true;
        return;
      }
    }
    if(_currentBuffer == 1) {
      _buffer0->readTransfer();
    }
    else {
      _buffer1->readTransfer();
    }
  }

  template<typename UserType>
  void DoubleBufferAccessor<UserType>::doPostRead(TransferType type, bool hasNewData) {
    auto unlocker = cppext::finally([&] { _transferLock.unlock(); });
    if(_transferSkipped) {
      // doReadTransferSynchronously() skipped the physical buffer read because the gate is closed, so the wrapped
      // buffer accessor was never read and its postRead must not be invoked (its buffer is stale). Re-enable the
      // firmware buffer swapping that doPreRead() disabled, to keep the double-buffer handshake balanced, and report
      // the read as faulty and not-new (the validity was already set by doReadTransferSynchronously()).
      _transferSkipped = false;
      if(_mutex->useCount() == 1) {
        _enableDoubleBufferReg->accessData(0) = 1;
        _enableDoubleBufferReg->write();
      }
      this->_versionNumber = {};
      return;
    }
    if(_currentBuffer == 1) {
      _buffer0->postRead(type, hasNewData);
    }
    else {
      _buffer1->postRead(type, hasNewData);
    }

    // release a lock in firmware (enable buffer swapping)
    if(_mutex->useCount() == 1) {
      _enableDoubleBufferReg->accessData(0) = 1;
      _enableDoubleBufferReg->write();
    }
    // set version and data validity of this object
    this->_versionNumber = {};
    if(_currentBuffer == 1) {
      this->_dataValidity = _buffer0->dataValidity();
    }
    else {
      this->_dataValidity = _buffer1->dataValidity();
    }

    // Note: TransferElement Spec E.6.1 dictates that the version number and data validity needs to be set before this
    // check.
    if(!hasNewData) {
      return;
    }

    // Swap buffer_2D if new data
    if(hasNewData) {
      auto& reg = (_currentBuffer == 1) ? _buffer0 : _buffer1;
      for(size_t i = 0; i < reg->getNumberOfChannels(); ++i) {
        buffer_2D[i].swap(reg->accessChannel(i));
      }
    }
  }

  template<typename UserType>
  std::vector<boost::shared_ptr<TransferElement>> DoubleBufferAccessor<UserType>::getHardwareAccessingElements() {
    // returning only this means the DoubleBufferAccessor will not be optimized when put into TransferGroup
    // optimizing would break our handshake protocol, since it reorders transfers
    return {TransferElement::shared_from_this()};
  }

  template<typename UserType>
  bool DoubleBufferAccessor<UserType>::mayReplaceOther(const boost::shared_ptr<const TransferElement>& other) const {
    auto otherDoubleBuffer = boost::dynamic_pointer_cast<const DoubleBufferAccessor<UserType>>(other);
    if(!otherDoubleBuffer || otherDoubleBuffer.get() == this) {
      return false;
    }
    return (_buffer0->mayReplaceOther(otherDoubleBuffer->_buffer0));
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DoubleBufferAccessor);

} // namespace ChimeraTK
