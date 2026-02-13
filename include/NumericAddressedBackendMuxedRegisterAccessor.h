// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Exception.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedBackend.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "RawConverter.h"

#include <boost/shared_ptr.hpp>

namespace ChimeraTK {

  constexpr auto MULTIPLEXED_SEQUENCE_PREFIX = "AREA_MULTIPLEXED_SEQUENCE_";
  constexpr auto SEQUENCE_PREFIX = "SEQUENCE_";

  constexpr auto MEM_MULTIPLEXED_PREFIX = "MEM_MULTIPLEXED_";

  /********************************************************************************************************************/

  namespace detail {

    /** Iteration on a raw buffer with a given pitch (increment from one element to the next) in bytes */
    template<typename DATA_TYPE>
    struct pitched_iterator {
      // standard iterator traits
      using iterator_category = std::random_access_iterator_tag;
      using value_type = DATA_TYPE;
      using difference_type = std::ptrdiff_t;
      using pointer = DATA_TYPE*;
      using reference = DATA_TYPE&;

      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      pitched_iterator(void* begin, size_t pitch) : _ptr(reinterpret_cast<std::byte*>(begin)), _pitch(pitch) {}

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
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      DATA_TYPE& operator*() const { return *reinterpret_cast<DATA_TYPE*>(_ptr); }

     private:
      std::byte* _ptr;
      const size_t _pitch;

      template<typename OTHER_DATA_TYPE>
      friend struct pitched_iterator;
    };

  } // namespace detail

  /********************************************************************************************************************/
  /**
   * Implementation of the NDRegisterAccessor for NumericAddressedBackends for multiplexd 2D registers
   */
  template<class UserType>
  class NumericAddressedBackendMuxedRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendMuxedRegisterAccessor(const RegisterPath& registerPathName, size_t numberOfElements,
        size_t elementsOffset, const boost::shared_ptr<DeviceBackend>& _backend);

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    template<typename UserType2, typename RawType, RawConverter::SignificantBitsCase sc,
        RawConverter::FractionalCase fc, bool isSigned>
    void doPostReadImpl(RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelIndex);

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    template<typename UserType2, typename RawType, RawConverter::SignificantBitsCase sc,
        RawConverter::FractionalCase fc, bool isSigned>
    void doPreWriteImpl(RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelIndex);

    void doPreRead(TransferType) override {
      if(!_ioDevice->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    }

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedBackendMuxedRegisterAccessor<UserType>>(other);
      if(rhsCasted.get() == this) {
        return false;
      }
      if(!rhsCasted) return false;
      if(_ioDevice != rhsCasted->_ioDevice) return false;
      if(_registerInfo != rhsCasted->_registerInfo) return false;
      // No need to compare converters, since they are derived from registerInfo and UserType.
      return true;
    }

    [[nodiscard]] bool isReadOnly() const override { return isReadable() && !isWriteable(); }

    [[nodiscard]] bool isReadable() const override { return _registerInfo.isReadable(); }

    [[nodiscard]] bool isWriteable() const override { return _registerInfo.isWriteable(); }

   protected:
    std::vector<std::unique_ptr<RawConverter::ConverterLoopHelper>> _converterLoopHelpers;

    /** The device from (/to) which to perform the DMA transfer */
    boost::shared_ptr<NumericAddressedBackend> _ioDevice;

    std::vector<int32_t> _ioBuffer;

    NumericAddressedRegisterInfo _registerInfo;

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

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendMuxedRegisterAccessor);

  /********************************************************************************************************************/

} // namespace ChimeraTK
