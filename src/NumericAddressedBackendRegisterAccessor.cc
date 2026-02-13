// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  NumericAddressedBackendRegisterAccessor<UserType, isRaw>::NumericAddressedBackendRegisterAccessor(
      const boost::shared_ptr<DeviceBackend>& dev, const RegisterPath& registerPathName, size_t numberOfWords,
      size_t wordOffsetInRegister, AccessModeFlags flags)
  : NDRegisterAccessor<UserType>(registerPathName, flags),
    _dev(boost::dynamic_pointer_cast<NumericAddressedBackend>(dev)) {
    // check for unknown flags
    flags.checkForUnknownFlags({AccessMode::raw});

    // check device backend
    _dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev);
    if(!_dev) {
      throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor is used with a backend which is not "
                                   "a NumericAddressedBackend.");
    }

    // obtain register information
    _registerInfo = _dev->getRegisterInfo(registerPathName);
    assert(!_registerInfo.channels.empty());

    if(_registerInfo.elementPitchBits % 8 != 0) {
      throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor: Elements must be byte aligned.");
    }

    if(_registerInfo.channels.size() > 1) {
      throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor is used with a 2D register.");
    }

    if(_registerInfo.channels.front().bitOffset > 0) {
      throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor: Registers must be byte aligned.");
    }

    // check number of words
    if(_registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::VOID) {
      // in void registers we always create one element
      if(numberOfWords == 0) {
        numberOfWords = 1;
      }
      if(numberOfWords > 1) {
        throw ChimeraTK::logic_error(
            "Requested number of words is larger than 1 in VOID register '" + registerPathName + "'!");
      }
      if(wordOffsetInRegister > 0) {
        throw ChimeraTK::logic_error("No offset allowed in VOID register '" + registerPathName + "'!");
      }
    }
    else { // do the regular consistency check
      if(numberOfWords == 0) {
        numberOfWords = _registerInfo.getNumberOfElements();
      }
      if(numberOfWords + wordOffsetInRegister > _registerInfo.getNumberOfElements()) {
        throw ChimeraTK::logic_error("Requested number of words (" + std::to_string(numberOfWords) + " + " +
            std::to_string(wordOffsetInRegister) + ") exceeds the size (" +
            std::to_string(_registerInfo.getNumberOfElements()) + ") of the register '" + registerPathName + "'!");
      }
      if(wordOffsetInRegister >= _registerInfo.getNumberOfElements()) {
        throw ChimeraTK::logic_error("Requested offset (" + std::to_string(wordOffsetInRegister) +
            ") exceeds the size (" + std::to_string(_registerInfo.getNumberOfElements()) + ") of the register'" +
            registerPathName + "'!");
      }
    }

    // change registerInfo (local copy!) to account for given offset and length override
    _registerInfo.address += wordOffsetInRegister * _registerInfo.elementPitchBits / 8;
    _registerInfo.nElements = numberOfWords;

    // create low-level transfer element handling the actual data transfer to the hardware with raw data
    assert(_registerInfo.elementPitchBits % 8 == 0);
    _rawAccessor = boost::make_shared<NumericAddressedLowLevelTransferElement>(
        _dev, _registerInfo.bar, _registerInfo.address, _registerInfo.nElements * _registerInfo.elementPitchBits / 8);

    // allocated the buffers
    NDRegisterAccessor<UserType>::buffer_2D.resize(1);
    NDRegisterAccessor<UserType>::buffer_2D[0].resize(_registerInfo.nElements);

    if constexpr(!isRaw) {
      _converterLoopHelper =
          RawConverter::ConverterLoopHelper::makeConverterLoopHelper<UserType>(_registerInfo, 0, *this);
    }

    if(flags.has(AccessMode::raw)) {
      if(DataType(typeid(UserType)) != _registerInfo.getDataDescriptor().rawDataType()) {
        throw ChimeraTK::logic_error("Given UserType when obtaining the NumericAddressedBackendRegisterAccessor in "
                                     "raw mode does not match the expected type. Use an " +
            _registerInfo.getDataDescriptor().rawDataType().getAsString() +
            " instead! (Register name: " + registerPathName + "')");
      }
    }

    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doReadTransferSynchronously() {
    _rawAccessor->readTransfer();
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  bool NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doWriteTransfer(
      ChimeraTK::VersionNumber versionNumber) {
    assert(!TransferElement::_isInTransferGroup);
    _rawAccessor->writeTransfer(versionNumber);
    return false;
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doPostRead(TransferType type, bool hasNewData) {
    if(!_dev->isOpen()) return; // do not delegate if exception was thrown by us in doPreWrite

    _rawAccessor->setActiveException(this->_activeException);
    _rawAccessor->postRead(type, hasNewData);

    if(!hasNewData) return;

    if constexpr(!isRaw || std::is_same<UserType, std::string>::value) {
      _converterLoopHelper->doPostRead();
    }
    else {
      // optimised variant for raw transfers (unless type is a string)
      auto* itsrc = _rawAccessor->begin(_registerInfo.address);
      auto* itdst = buffer_2D[0].data();
      memcpy(itdst, itsrc, buffer_2D[0].size() * sizeof(UserType));
    }

    // we don't put the setting of the version number into the PrePostActionImplementor
    // because it does not need template specialisation, and the implementer does not
    // know about _versionNumber. It's just easier here.
    this->_versionNumber = _rawAccessor->getVersionNumber();
    this->_dataValidity = _rawAccessor->dataValidity();
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doPostReadImpl(
      RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex) {
    auto* begin = reinterpret_cast<RawType*>(_rawAccessor->begin(_registerInfo.address));
    for(auto [itsrc, itdst] = std::make_pair(begin, buffer_2D[0].begin()); itdst != buffer_2D[0].end();
        ++itsrc, ++itdst) {
      *itdst = converter.toCooked(*itsrc);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doPreWrite(
      TransferType type, VersionNumber versionNumber) {
    if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    // raw accessor preWrite must be called before our _prePostActionsImplementor.doPreWrite(), as it needs to
    // prepare the buffer in case of unaligned access and acquire the lock.
    _rawAccessor->preWrite(type, versionNumber);

    if constexpr(!isRaw || std::is_same<UserType, std::string>::value) {
      if(!_registerInfo.isWriteable()) {
        throw ChimeraTK::logic_error(
            "NumericAddressedBackend: Writing to a non-writeable register is not allowed (Register name: " +
            _registerInfo.getRegisterName() + ").");
      }
      _converterLoopHelper->doPreWrite();
    }
    else {
      // optimised variant for raw transfers (unless type is a string)
      auto* itdst = _rawAccessor->begin(_registerInfo.address);
      auto itsrc = buffer_2D[0].begin();
      memcpy(&(*itdst), &(*itsrc), buffer_2D[0].size() * sizeof(UserType));
    }

    _rawAccessor->setDataValidity(this->_dataValidity);
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doPreWriteImpl(
      RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex) {
    auto* begin = reinterpret_cast<RawType*>(_rawAccessor->begin(_registerInfo.address));
    for(auto [itsrc, itdst] = std::make_pair(buffer_2D[0].begin(), begin); itsrc != buffer_2D[0].end();
        ++itsrc, ++itdst) {
      *itdst = converter.toRaw(*itsrc);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doPreRead(TransferType type) {
    if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    if(!_registerInfo.isReadable()) {
      throw ChimeraTK::logic_error(
          "NumericAddressedBackend: Reading from a non-readable register is not allowed (Register name: " +
          _registerInfo.getRegisterName() + ").");
    }
    _rawAccessor->preRead(type);
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::doPostWrite(
      TransferType type, VersionNumber versionNumber) {
    if(!_dev->isOpen()) return; // do not delegate if exception was thrown by us in doPreWrite
    _rawAccessor->setActiveException(this->_activeException);
    _rawAccessor->postWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  bool NumericAddressedBackendRegisterAccessor<UserType, isRaw>::mayReplaceOther(
      const boost::shared_ptr<TransferElement const>& other) const {
    auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedBackendRegisterAccessor<UserType, isRaw>>(other);
    if(rhsCasted.get() == this) {
      return false;
    }
    if(!rhsCasted) return false;
    if(_dev != rhsCasted->_dev) return false;
    if(_registerInfo != rhsCasted->_registerInfo) return false;
    // No need to compare the RawConverters, since they are based on the registerInfo and UserType only.
    return true;
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  bool NumericAddressedBackendRegisterAccessor<UserType, isRaw>::isReadOnly() const {
    return isReadable() && !isWriteable();
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  bool NumericAddressedBackendRegisterAccessor<UserType, isRaw>::isReadable() const {
    return _registerInfo.isReadable();
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  bool NumericAddressedBackendRegisterAccessor<UserType, isRaw>::isWriteable() const {
    return _registerInfo.isWriteable();
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::setExceptionBackend(
      boost::shared_ptr<DeviceBackend> exceptionBackend) {
    this->_exceptionBackend = exceptionBackend;
    _rawAccessor->setExceptionBackend(exceptionBackend);
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  std::vector<boost::shared_ptr<TransferElement>> NumericAddressedBackendRegisterAccessor<UserType,
      isRaw>::getHardwareAccessingElements() {
    return _rawAccessor->getHardwareAccessingElements();
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  std::list<boost::shared_ptr<TransferElement>> NumericAddressedBackendRegisterAccessor<UserType,
      isRaw>::getInternalElements() {
    return {_rawAccessor}; // the rawAccessor always returns an empty list
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::replaceTransferElement(
      boost::shared_ptr<TransferElement> newElement) {
    auto casted = boost::dynamic_pointer_cast<NumericAddressedLowLevelTransferElement>(newElement);
    if(casted && casted->isMergeable(_rawAccessor)) {
      size_t newStartAddress = std::min(casted->_startAddress, _rawAccessor->_startAddress);
      size_t newStopAddress = std::max(
          casted->_startAddress + casted->_numberOfBytes, _rawAccessor->_startAddress + _rawAccessor->_numberOfBytes);
      size_t newNumberOfBytes = newStopAddress - newStartAddress;
      casted->changeAddress(newStartAddress, newNumberOfBytes);
      _rawAccessor = casted;
    }
    _rawAccessor->setExceptionBackend(this->_exceptionBackend);
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<typename COOKED_TYPE>
  COOKED_TYPE NumericAddressedBackendRegisterAccessor<UserType, isRaw>::getAsCooked_impl(
      unsigned int channel, unsigned int sample) {
    if constexpr(isRaw && std::is_integral_v<UserType>) {
      if constexpr(isRawType<std::make_unsigned_t<UserType>>) {
        // Note: the "UserType" is our RawType here!
        COOKED_TYPE rv;
        RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<UserType>>(_registerInfo, 0, [&](auto converter) {
          rv = converter.toCooked(
              std::make_unsigned_t<UserType>(NDRegisterAccessor<UserType>::buffer_2D[channel][sample]));
        });
        return rv;
      }
    }
    throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<typename COOKED_TYPE>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::setAsCooked_impl(
      unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    if constexpr(isRaw && std::is_integral_v<UserType>) {
      if constexpr(isRawType<std::make_unsigned_t<UserType>>) {
        // Note: the "UserType" is our RawType here!
        RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<UserType>>(_registerInfo, 0, [&](auto converter) {
          NDRegisterAccessor<UserType>::buffer_2D[channel][sample] = UserType(converter.toRaw(value));
        });
      }
      return;
    }
    throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
  }

  /********************************************************************************************************************/

  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, true);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, false);

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
