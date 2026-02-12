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
    _mutex(mutex) {
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

    // enable double buffering??
    // uint32_t enable = 1;
    // _enableDoubleBufferReg->accessChannel(0)[0] = enable;
    // _enableDoubleBufferReg->write();
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPreRead(TransferType type) {
    {
      std::lock_guard<detail::CountedRecursiveMutex> lg(*_mutex);
      if(_mutex->useCount() == 1) {
        // acquire a lock in firmware (disable buffer swapping)
        _enableDoubleBufferReg->accessData(0) = 0;
        _enableDoubleBufferReg->write();
      }
    }
    // check which buffer is now in use by the firmware
    _currentBufferNumberReg->read();
    _currentBuffer = _currentBufferNumberReg->accessData(0);
    // if current buffer 1, it means firmware writes now to buffer1, so use target (buffer 0), else use
    // _secondBufferReg (buffer 1)
    if(_currentBuffer) {
      _target->preRead(type);
    }
    else {
      _secondBufferReg->preRead(type);
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doReadTransferSynchronously() {
    if(_currentBuffer) {
      _target->readTransfer();
    }
    else {
      _secondBufferReg->readTransfer();
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(_currentBuffer) {
      _target->postRead(type, hasNewData);
    }
    else {
      _secondBufferReg->postRead(type, hasNewData);
    }

    {
      std::lock_guard<detail::CountedRecursiveMutex> lg(*_mutex);
      if(_mutex->useCount() == 1) {
        // acquire a lock in firmware (disable buffer swapping)
        _enableDoubleBufferReg->accessData(0) = 1;
        _enableDoubleBufferReg->write();
      }
    }
    // set version and data validity of this object
    this->_versionNumber = {};
    if(_currentBuffer) {
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

    if(_currentBuffer) {
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(_target->accessChannel(i));
      }
    }
    else {
      for(size_t i = 0; i < _secondBufferReg->getNumberOfChannels(); ++i) {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(_secondBufferReg->accessChannel(i));
      }
    }
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

  // Explicit template instantiations
#define NUMERIC_DOUBLE_BUFFER_TYPES                                                                                    \
  X(int8_t)                                                                                                            \
  X(uint8_t)                                                                                                           \
  X(int16_t)                                                                                                           \
  X(uint16_t)                                                                                                          \
  X(int32_t)                                                                                                           \
  X(uint32_t)                                                                                                          \
  X(uint64_t)                                                                                                          \
  X(long)                                                                                                              \
  X(float)                                                                                                             \
  X(double)                                                                                                            \
  X(ChimeraTK::Void)                                                                                                   \
  X(std::string)                                                                                                       \
  X(ChimeraTK::Boolean)

#define X(T) template class ChimeraTK::DoubleBufferAccessorDecorator<T>;
  NUMERIC_DOUBLE_BUFFER_TYPES
#undef X

} // namespace ChimeraTK
