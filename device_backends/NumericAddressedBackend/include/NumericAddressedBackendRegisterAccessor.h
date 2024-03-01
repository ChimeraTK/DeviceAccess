// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "createDataConverter.h"
#include "FixedPointConverter.h"
#include "ForwardDeclarations.h"
#include "IEEE754_SingleConverter.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"

#include <ChimeraTK/cppext/finally.hpp>

namespace ChimeraTK {

  template<typename UserType, typename DataConverterType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor;

  /*********************************************************************************************************************/

  /**
   * Implementation of the NDRegisterAccessor for NumericAddressedBackends for scalar and 1D registers.
   */
  template<typename UserType, typename DataConverterType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendRegisterAccessor(const boost::shared_ptr<DeviceBackend>& dev,
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : NDRegisterAccessor<UserType>(registerPathName, flags), _dataConverter(registerPathName),
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
          throw ChimeraTK::logic_error(
              "Requested number of words exceeds the size of the register '" + registerPathName + "'!");
        }
        if(wordOffsetInRegister >= _registerInfo.getNumberOfElements()) {
          throw ChimeraTK::logic_error("Requested offset exceeds the size of the register'" + registerPathName + "'!");
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

      // We don't have to fill it in a special way if the accessor is raw
      // because we have an overloaded, more efficient implementation
      // in this case. So we can use it in setAsCooked() and getAsCooked()
      _dataConverter = detail::createDataConverter<DataConverterType>(_registerInfo);

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

    void doReadTransferSynchronously() override { _rawAccessor->readTransfer(); }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
      assert(!TransferElement::_isInTransferGroup);
      _rawAccessor->writeTransfer(versionNumber);
      return false;
    }

    void doPostRead(TransferType type, bool hasNewData) override {
      if(!_dev->isOpen()) return; // do not delegate if exception was thrown by us in doPreWrite

      _rawAccessor->setActiveException(this->_activeException);
      _rawAccessor->postRead(type, hasNewData);

      if(!hasNewData) return;

      if constexpr(!isRaw || std::is_same<UserType, std::string>::value) {
        callForRawType(_registerInfo.getDataDescriptor().rawDataType(), [this](auto t) {
          typedef decltype(t) RawType;
          auto itsrc = (RawType*)_rawAccessor->begin(_registerInfo.address);
          _dataConverter.template vectorToCooked<UserType>(itsrc, itsrc + buffer_2D[0].size(), buffer_2D[0].begin());
        });
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

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      // raw accessor preWrite must be called before our _prePostActionsImplementor.doPreWrite(), as it needs to
      // prepare the buffer in case of unaligned access and acquire the lock.
      _rawAccessor->preWrite(type, versionNumber);

      if constexpr(!isRaw || std::is_same<UserType, std::string>::value) {
        if(!_registerInfo.isWriteable()) {
          throw ChimeraTK::logic_error(
              "NumericAddressedBackend: Writeing to a non-writeable register is not allowed (Register name: " +
              _registerInfo.getRegisterName() + ").");
        }
        callForRawType(_registerInfo.getDataDescriptor().rawDataType(), [this](auto t) {
          typedef decltype(t) RawType;
          auto itsrc = (RawType*)_rawAccessor->begin(_registerInfo.address);
          for(auto itdst = buffer_2D[0].begin(); itdst != buffer_2D[0].end(); ++itdst) {
            *itsrc = _dataConverter.template toRaw<UserType>(*itdst);
            ++itsrc;
          }
        });
      }
      else {
        // optimised variant for raw transfers (unless type is a string)
        auto* itdst = _rawAccessor->begin(_registerInfo.address);
        auto itsrc = buffer_2D[0].begin();
        memcpy(&(*itdst), &(*itsrc), buffer_2D[0].size() * sizeof(UserType));
      }

      _rawAccessor->setDataValidity(this->_dataValidity);
    }

    void doPreRead(TransferType type) override {
      if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      _rawAccessor->preRead(type);
    }

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      if(!_dev->isOpen()) return; // do not delegate if exception was thrown by us in doPreWrite
      _rawAccessor->setActiveException(this->_activeException);
      _rawAccessor->postWrite(type, versionNumber);
    }

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<
          const NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>>(other);
      if(!rhsCasted) return false;
      if(_dev != rhsCasted->_dev) return false;
      if(_registerInfo != rhsCasted->_registerInfo) return false;
      if(_dataConverter != rhsCasted->_dataConverter) return false;
      return true;
    }

    [[nodiscard]] bool isReadOnly() const override { return isReadable() && !isWriteable(); }

    [[nodiscard]] bool isReadable() const override { return _registerInfo.isReadable(); }

    [[nodiscard]] bool isWriteable() const override { return _registerInfo.isWriteable(); }

    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample);

    template<typename COOKED_TYPE>
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value);

    // a local typename so the DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER does
    // not get confused by the comma which separates the two template parameters
    using THIS_TYPE = NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, setAsCooked_impl, 3);

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
      _rawAccessor->setExceptionBackend(exceptionBackend);
    }

   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    NumericAddressedRegisterInfo _registerInfo;

    /** Converter to interpret the data */
    DataConverterType _dataConverter;

    /** raw accessor */
    boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<NumericAddressedBackend> _dev;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return _rawAccessor->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      return {_rawAccessor}; // the rawAccessor always returns an empty list
    }

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
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

    /**
     * A helper class to implement template specialisation on certain functions. We can do a partial specialisation on
     * this class which we cannot/don't want to do for the whole accessor.
     */
    template<typename RawT, typename CookedT>
    struct dataConverterTemplateSpecialisationHelper {
      static CookedT vectorToCooked(DataConverterType&, const typename std::vector<RawT>::const_iterator&,
          const typename std::vector<RawT>::const_iterator&, const typename std::vector<CookedT>::iterator&) {
        throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
      }
      static RawT toRaw(DataConverterType&, CookedT&) {
        throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
      }
    };
    template<typename CookedT>
    struct dataConverterTemplateSpecialisationHelper<int8_t, CookedT> {
      static void vectorToCooked(DataConverterType& dataConverter,
          const typename std::vector<int8_t>::const_iterator& start,
          const typename std::vector<int8_t>::const_iterator& end,
          const typename std::vector<CookedT>::iterator& cooked) {
        dataConverter.template vectorToCooked<CookedT>(start, end, cooked);
      }
      static int8_t toRaw(DataConverterType& dataConverter, CookedT& value) { return dataConverter.toRaw(value); }
    };
    template<typename CookedT>
    struct dataConverterTemplateSpecialisationHelper<int16_t, CookedT> {
      static void vectorToCooked(DataConverterType& dataConverter,
          const typename std::vector<int16_t>::const_iterator& start,
          const typename std::vector<int16_t>::const_iterator& end,
          const typename std::vector<CookedT>::iterator& cooked) {
        dataConverter.template vectorToCooked<CookedT>(start, end, cooked);
      }
      static int16_t toRaw(DataConverterType& dataConverter, CookedT& value) { return dataConverter.toRaw(value); }
    };
    template<typename CookedT>
    struct dataConverterTemplateSpecialisationHelper<int32_t, CookedT> {
      static void vectorToCooked(DataConverterType& dataConverter,
          const typename std::vector<int32_t>::const_iterator& start,
          const typename std::vector<int32_t>::const_iterator& end,
          const typename std::vector<CookedT>::iterator& cooked) {
        dataConverter.template vectorToCooked<CookedT>(start, end, cooked);
      }
      static int32_t toRaw(DataConverterType& dataConverter, CookedT& value) { return dataConverter.toRaw(value); }
    };

    using NDRegisterAccessor<UserType>::buffer_2D;
  }; // namespace ChimeraTK

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType, typename DataConverterType, bool isRaw>
  template<typename COOKED_TYPE>
  COOKED_TYPE NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>::getAsCooked_impl(
      unsigned int channel, unsigned int sample) {
    if(isRaw) {
      std::vector<COOKED_TYPE> cookedData(1);
      dataConverterTemplateSpecialisationHelper<UserType, COOKED_TYPE>::vectorToCooked(_dataConverter,
          NDRegisterAccessor<UserType>::buffer_2D[channel].begin() + sample,
          NDRegisterAccessor<UserType>::buffer_2D[channel].begin() + sample + 1, cookedData.begin());
      return cookedData[0];
    }
    throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType, typename DataConverterType, bool isRaw>
  template<typename COOKED_TYPE>
  void NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>::setAsCooked_impl(
      unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    if(isRaw) {
      NDRegisterAccessor<UserType>::buffer_2D[channel][sample] =
          dataConverterTemplateSpecialisationHelper<UserType, COOKED_TYPE>::toRaw(_dataConverter, value);
    }
    else {
      throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////

  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, FixedPointConverter, true);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, FixedPointConverter, false);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, true);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, false);

} // namespace ChimeraTK
