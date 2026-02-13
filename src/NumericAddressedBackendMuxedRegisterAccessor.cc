// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendMuxedRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<class UserType>
  NumericAddressedBackendMuxedRegisterAccessor<UserType>::NumericAddressedBackendMuxedRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfElements, size_t elementsOffset,
      const boost::shared_ptr<DeviceBackend>& _backend)
  : NDRegisterAccessor<UserType>(registerPathName, {}),
    _ioDevice(boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend)) {
    // Obtain information about the area
    _registerInfo = _ioDevice->_registerMap.getBackendRegister(registerPathName);
    assert(!_registerInfo.channels.empty());

    // Create a RawConverter for each channel
    for(size_t i = 0; i < _registerInfo.getNumberOfChannels(); ++i) {
      if(_registerInfo.channels[i].bitOffset % 8 != 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendMuxedRegisterAccessor: elements must be byte aligned.");
      }
      _converterLoopHelpers.emplace_back(
          RawConverter::ConverterLoopHelper::makeConverterLoopHelper<UserType>(_registerInfo, i, *this));
    }
    // check information
    if(_registerInfo.elementPitchBits % 8 != 0) {
      throw ChimeraTK::logic_error("NumericAddressedBackendMuxedRegisterAccessor: blocks must be byte aligned.");
    }

    // compute effective numberOfElements
    if(numberOfElements == 0) {
      numberOfElements = _registerInfo.nElements;
    }

    // check number of words
    if(numberOfElements + elementsOffset > _registerInfo.nElements) {
      throw ChimeraTK::logic_error("Requested number of elements (" + std::to_string(numberOfElements) + " + " +
          std::to_string(elementsOffset) + ") exceeds the size (" + std::to_string(_registerInfo.nElements) +
          ") of the register '" + registerPathName + "'!");

      throw ChimeraTK::logic_error("Requested number of elements exceeds the size of the register! Requested end: " +
          std::to_string(numberOfElements + elementsOffset) +
          ", register length: " + std::to_string(_registerInfo.nElements));
    }

    // update register info
    _registerInfo.nElements = numberOfElements;
    assert(_registerInfo.elementPitchBits % 8 == 0);
    _registerInfo.address += elementsOffset * _registerInfo.elementPitchBits / 8;

    // allocate the buffer for the converted data
    NDRegisterAccessor<UserType>::buffer_2D.resize(_converterLoopHelpers.size());
    for(size_t i = 0; i < _converterLoopHelpers.size(); ++i) {
      NDRegisterAccessor<UserType>::buffer_2D[i].resize(_registerInfo.nElements);
    }

    // allocate the raw io buffer. Make it one element larger to make sure we can access the last byte via int32_t*
    _ioBuffer.resize(
        static_cast<size_t>(_registerInfo.elementPitchBits) / 8 * _registerInfo.nElements / sizeof(int32_t) + 1);

    // compute pitched iterators for accessing the channels
    // Silence the linter: Yes, we are doing a reinterpet cast. There is nothing we can do about it when we're
    // bit-fiddling

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* ioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
    for(auto& c : _registerInfo.channels) {
      assert(c.bitOffset % 8 == 0);
      _startIterators.emplace_back(ioBuffer + c.bitOffset / 8, _registerInfo.elementPitchBits / 8);
      _endIterators.push_back(_startIterators.back() + _registerInfo.nElements);
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doReadTransferSynchronously() {
    assert(_registerInfo.elementPitchBits % 8 == 0);

    auto nbt = _registerInfo.elementPitchBits / 8 * _registerInfo.nElements;
    nbt = ((nbt - 1) / 4 + 1) * 4; // round up to multiple of 4 bytes

    _ioDevice->read(_registerInfo.bar, _registerInfo.address, _ioBuffer.data(), nbt);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      for(size_t channelIndex = 0; channelIndex < _registerInfo.channels.size(); ++channelIndex) {
        _converterLoopHelpers[channelIndex]->doPostRead();
      }

      // it is acceptable to create the version number in post read because this accessor does not have
      // wait_for_new_data. It is basically synchronous.
      this->_versionNumber = {};

      // we just read good data. Set validity back to ok if someone marked it faulty for writing.
      this->_dataValidity = DataValidity::ok;
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPostReadImpl(
      RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelIndex) {
    static_assert(std::is_same_v<UserType, UserType2>);
    if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
      std::transform(_startIterators[channelIndex], _endIterators[channelIndex], buffer_2D[channelIndex].begin(),
          [&](auto rawValue) { return converter.toCooked(rawValue); });
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  bool NumericAddressedBackendMuxedRegisterAccessor<UserType>::doWriteTransfer(VersionNumber) {
    assert(_registerInfo.elementPitchBits % 8 == 0);

    auto nbt = _registerInfo.elementPitchBits / 8 * _registerInfo.nElements;
    nbt = ((nbt - 1) / 4 + 1) * 4; // round up to multiple of 4 bytes

    _ioDevice->write(_registerInfo.bar, _registerInfo.address, &(_ioBuffer[0]), nbt);
    return false;
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPreWrite(TransferType, VersionNumber) {
    if(!_ioDevice->isOpen()) {
      throw ChimeraTK::logic_error("Device not opened.");
    }

    assert(_registerInfo.channels.size() == _converterLoopHelpers.size());

    for(size_t channelIndex = 0; channelIndex < _registerInfo.channels.size(); ++channelIndex) {
      _converterLoopHelpers[channelIndex]->doPreWrite();
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPreWriteImpl(
      RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelIndex) {
    if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
      std::transform(buffer_2D[channelIndex].begin(), buffer_2D[channelIndex].end(),
          detail::pitched_iterator<RawType>(_startIterators[channelIndex]),
          [&](UserType cookedValue) { return converter.toRaw(cookedValue); });
    }
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendMuxedRegisterAccessor);
} // namespace ChimeraTK
