#pragma once

#include <boost/shared_ptr.hpp>
#include <sstream>

#include "Exception.h"
#include "FixedPointConverter.h"
#include "MapFileParser.h"
#include "NumericAddressedBackend.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "NDRegisterAccessor.h"

namespace ChimeraTK {

  static const std::string MULTIPLEXED_SEQUENCE_PREFIX = "AREA_MULTIPLEXED_SEQUENCE_";
  static const std::string SEQUENCE_PREFIX = "SEQUENCE_";

  /********************************************************************************************************************/

  namespace detail {

    /** Iteration on a raw buffer with a given pitch (increment from one element to the next) in bytes */
    template<typename DATA_TYPE>
    struct pitched_iterator : std::iterator<std::random_access_iterator_tag, DATA_TYPE> {
      pitched_iterator(void* begin, size_t pitch) : _ptr(reinterpret_cast<uint8_t*>(begin)), _pitch(pitch) {}

      template<typename OTHER_DATA_TYPE>
      explicit pitched_iterator(pitched_iterator<OTHER_DATA_TYPE>& other) : _ptr(other._ptr), _pitch(other._pitch) {}

      pitched_iterator& operator++() {
        _ptr += _pitch;
        return *this;
      }
      pitched_iterator operator++(int) {
        pitched_iterator retval = *this;
        ++(*this);
        return retval;
      }
      pitched_iterator operator+(size_t n) { return pitched_iterator(_ptr + n * _pitch, _pitch); }
      bool operator==(pitched_iterator other) const { return _ptr == other._ptr; }
      bool operator!=(pitched_iterator other) const { return !(*this == other); }
      size_t operator-(pitched_iterator other) const { return _ptr - other._ptr; }
      DATA_TYPE& operator*() const { return *reinterpret_cast<DATA_TYPE*>(_ptr); }

     private:
      uint8_t* _ptr;
      const size_t _pitch;

      template<typename OTHER_DATA_TYPE>
      friend struct pitched_iterator;
    };

  } // namespace detail

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for
   * multiplexd 2D registers
   */
  template<class UserType>
  class NumericAddressedBackendMuxedRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendMuxedRegisterAccessor(const RegisterPath& registerPathName, size_t numberOfElements,
        size_t elementsOffset, boost::shared_ptr<DeviceBackend> _backend);

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    void doPreRead(TransferType) override {
      if(!_ioDevice->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedBackendMuxedRegisterAccessor<UserType>>(other);
      if(!rhsCasted) return false;
      if(_ioDevice != rhsCasted->_ioDevice) return false;
      if(_registerInfo != rhsCasted->_registerInfo) return false;
      if(_converters != rhsCasted->_converters) return false;
      return true;
    }

    bool isReadOnly() const override { return isReadable() && !isWriteable(); }

    bool isReadable() const override { return _registerInfo.isReadable(); }

    bool isWriteable() const override { return _registerInfo.isWriteable(); }

   protected:
    /** One fixed point converter for each sequence. */
    std::vector<FixedPointConverter> _converters;

    /** The device from (/to) which to perform the DMA transfer */
    boost::shared_ptr<NumericAddressedBackend> _ioDevice;

    std::vector<int32_t> _ioBuffer;

    NumericAddressedRegisterInfo _registerInfo;
    size_t _numberOfElements;
    size_t _elementsOffset;

    std::vector<detail::pitched_iterator<int32_t>> _startIterators;
    std::vector<detail::pitched_iterator<int32_t>> _endIterators;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    using NDRegisterAccessor<UserType>::buffer_2D;
    using TransferElement::_exceptionBackend;
  };

  /********************************************************************************************************************/

  template<class UserType>
  NumericAddressedBackendMuxedRegisterAccessor<UserType>::NumericAddressedBackendMuxedRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfElements, size_t elementsOffset,
      boost::shared_ptr<DeviceBackend> _backend)
  : NDRegisterAccessor<UserType>(registerPathName, {}),
    _ioDevice(boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend)), _numberOfElements(numberOfElements),
    _elementsOffset(elementsOffset) {
    // Obtain information about the area
    _registerInfo = _ioDevice->_registerMap.getBackendRegister(registerPathName);
    assert(!_registerInfo.channels.empty());

    // Create a fixed point converter for each channel
    for(auto& c : _registerInfo.channels) {
      if(c.bitOffset % 8 != 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendMuxedRegisterAccessor: elements must be byte aligned.");
      }
      _converters.emplace_back(registerPathName, c.width, c.nFractionalBits, c.signedFlag);
    }

    // check information
    if(_registerInfo.elementPitchBits % 8 != 0) {
      throw ChimeraTK::logic_error("NumericAddressedBackendMuxedRegisterAccessor: blocks must be byte aligned.");
    }

    // check number of words
    if(_numberOfElements == 0) {
      _numberOfElements = _registerInfo.nElements;
    }
    if(_numberOfElements + _elementsOffset > _registerInfo.nElements) {
      throw ChimeraTK::logic_error("Requested number of elements exceeds the size of the register! Requested end: " +
          std::to_string(_numberOfElements + _elementsOffset) +
          ", register length: " + std::to_string(_registerInfo.nElements));
    }

    // allocate the buffer for the converted data
    NDRegisterAccessor<UserType>::buffer_2D.resize(_converters.size());
    for(size_t i = 0; i < _converters.size(); ++i) {
      NDRegisterAccessor<UserType>::buffer_2D[i].resize(_numberOfElements);
    }

    // allocate the raw io buffer. Make it one element larger to make sure we can access the last byte via int32_t*
    assert(_registerInfo.elementPitchBits % 8 == 0);
    _ioBuffer.resize(_registerInfo.elementPitchBits / 8 * _numberOfElements / sizeof(int32_t) + 1);

    // compute pitched iterators for accessing the channels
    auto* ioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
    for(auto& c : _registerInfo.channels) {
      assert(c.bitOffset % 8 == 0);
      _startIterators.emplace_back(ioBuffer + c.bitOffset / 8, _registerInfo.elementPitchBits / 8);
      _endIterators.push_back(_startIterators.back() + _numberOfElements);
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doReadTransferSynchronously() {
    assert(_registerInfo.elementPitchBits % 8 == 0);

    auto startAddress = _registerInfo.address + _elementsOffset * _registerInfo.elementPitchBits / 8;
    auto nbt = _registerInfo.elementPitchBits / 8 * _numberOfElements;
    nbt = ((nbt - 1) / 4 + 1) * 4; // round up to multiple of 4 bytes

    _ioDevice->read(_registerInfo.bar, startAddress, _ioBuffer.data(), nbt);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      for(size_t i = 0; i < _converters.size(); ++i) {
        _converters[i].template vectorToCooked<UserType>(_startIterators[i], _endIterators[i], buffer_2D[i].begin());
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
  bool NumericAddressedBackendMuxedRegisterAccessor<UserType>::doWriteTransfer(VersionNumber) {
    assert(_registerInfo.elementPitchBits % 8 == 0);

    auto startAddress = _registerInfo.address + _elementsOffset * _registerInfo.elementPitchBits / 8;
    auto nbt = _registerInfo.elementPitchBits / 8 * _numberOfElements;
    nbt = ((nbt - 1) / 4 + 1) * 4; // round up to multiple of 4 bytes

    _ioDevice->write(_registerInfo.bar, startAddress, &(_ioBuffer[0]), nbt);
    return false;
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPreWrite(TransferType, VersionNumber) {
    if(!_ioDevice->isOpen()) throw ChimeraTK::logic_error("Device not opened.");

    assert(_registerInfo.channels.size() == _converters.size());

    // Convert channel by channel
    for(size_t i = 0; i < _registerInfo.channels.size(); ++i) {
      callForRawType(_registerInfo.channels[i].getRawType(), [&](auto x) {
        using RawType = decltype(x);

        // Call FixedPointConverter::toRaw() for each value in the channel. The result (C++ type int32) is written to
        // the target buffer through the pitched iterators after converting it into the RawType matching the actual
        // bit width of the channel. This is important to avoid overwriting data of other channels.
        std::transform(buffer_2D[i].begin(), buffer_2D[i].end(), detail::pitched_iterator<RawType>(_startIterators[i]),
            [&](UserType cookedValue) { return _converters[i].toRaw(cookedValue); });
      });
    }
  }

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendMuxedRegisterAccessor);

} // namespace ChimeraTK
