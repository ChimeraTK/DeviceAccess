#include "NDDoubleBufferAccessorDecorator.h"
namespace ChimeraTK {

  template<typename UserType>
  NumericDoubleBufferAccessorDecorator<UserType>::NumericDoubleBufferAccessorDecorator(
      const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      std::optional<NumericAddressedRegisterInfo::DoubleBufferInfo> doubleBufferConfig,
      const boost::shared_ptr<DeviceBackend>& backend,
      std::shared_ptr<NumericAddressedBackend::DoubleBufferControlState> controlState)
  : NDRegisterAccessorDecorator<UserType>(target), _doubleBufferInfo(std::move(doubleBufferConfig)), _backend(backend),
    _controlState(controlState) {
    if(!_doubleBufferInfo) {
      throw ChimeraTK::logic_error("DoubleBufferInfo must be provided.");
    }
    _enableDoubleBufferReg =
        backend->getRegisterAccessor<uint32_t>(_doubleBufferInfo->enableRegisterPath, 1, _doubleBufferInfo->index, {});
    _currentBufferNumberReg = backend->getRegisterAccessor<uint32_t>(
        _doubleBufferInfo->inactiveBufferRegisterPath, 1, _doubleBufferInfo->index, {});

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
  void NumericDoubleBufferAccessorDecorator<UserType>::doPreRead(TransferType type) {
    {
      std::lock_guard<std::mutex> lg(_controlState->mutex);
      //_controlState._readerCount.value++;
      if(_controlState->readerCount == 0) {
        // acquire a lock in firmware (disable buffer swapping)
        _enableDoubleBufferReg->accessData(0) = 0;
        _enableDoubleBufferReg->write();
      }
      ++_controlState->readerCount;
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
  void NumericDoubleBufferAccessorDecorator<UserType>::doReadTransferSynchronously() {
    if(_currentBuffer) {
      _target->readTransfer();
    }
    else {
      _secondBufferReg->readTransfer();
    }
  }

  template<typename UserType>
  void NumericDoubleBufferAccessorDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(_currentBuffer) {
      _target->postRead(type, hasNewData);
    }
    else {
      _secondBufferReg->postRead(type, hasNewData);
    }

    {
      std::lock_guard lg{_controlState->mutex};
      assert(_controlState->readerCount > 0);
      _controlState->readerCount--;
      if(_controlState->readerCount == 0) {
        /*if(_testUSleep) {
          // for testing, check safety of handshake
          // FIXME - remove testUSleep feature
          _currentBufferNumberReg->read();
          if(_currentBuffer != _currentBufferNumberReg->accessData(0)) {
            std::cout << "WARNING: buffer switch happened while reading! Expect corrupted data." << std::endl;
          }
        }*/
        // release a lock in firmware (enable buffer swapping)
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
  std::vector<boost::shared_ptr<TransferElement>> NumericDoubleBufferAccessorDecorator<
      UserType>::getHardwareAccessingElements() {
    // returning only this means the DoubleBufferAccessorDecorator will not be optimized when put into TransferGroup
    // optimizing would break our handshake protocol, since it reorders transfers
    return {TransferElement::shared_from_this()};
  }

  template<typename UserType>
  bool NumericDoubleBufferAccessorDecorator<UserType>::mayReplaceOther(
      const boost::shared_ptr<const TransferElement>& other) const {
    // we need this to support merging of accessors using the same double-buffered as target.
    // If other is also double-buffered region belonging to the same plugin instance, allow the merge
    auto otherDoubleBuffer = boost::dynamic_pointer_cast<NumericDoubleBufferAccessorDecorator const>(other);
    if(!otherDoubleBuffer) {
      return false;
    }
    return &(otherDoubleBuffer->_controlState) == &_controlState;
  }

  // Explicit template instantiations
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<int8_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<uint8_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<int16_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<uint16_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<int32_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<uint32_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<uint64_t>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<long>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<float>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<double>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<ChimeraTK::Void>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<std::string>;
  template class ChimeraTK::NumericDoubleBufferAccessorDecorator<ChimeraTK::Boolean>;

} // namespace ChimeraTK
