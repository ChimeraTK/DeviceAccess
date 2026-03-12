// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "NDRegisterAccessorDecorator.h"
#include "RawConverter.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename TargetUserType>
  class FixedPointConvertingRawDecorator : public NDRegisterAccessorDecorator<TargetUserType> {
   public:
    FixedPointConvertingRawDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
        NumericAddressedRegisterInfo const& registerInfo)
    : NDRegisterAccessorDecorator<TargetUserType>(target), _registerInfo(registerInfo) {
      static_assert(isRawType<std::make_unsigned_t<TargetUserType>>);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    template<typename COOKED_TYPE>
    // NOLINTNEXTLINE(readability-identifier-naming)
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample) {
      COOKED_TYPE rv;
      RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<TargetUserType>>(
          _registerInfo, 0, [&](auto converter) {
            rv = converter.toCooked(
                std::make_unsigned_t<TargetUserType>(NDRegisterAccessor<TargetUserType>::buffer_2D[channel][sample]));
          });
      return rv;
    }

    template<typename COOKED_TYPE>
    // NOLINTNEXTLINE(readability-identifier-naming)
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value) {
      RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<TargetUserType>>(
          _registerInfo, 0, [&](auto converter) {
            NDRegisterAccessor<TargetUserType>::buffer_2D[channel][sample] = TargetUserType(converter.toRaw(value));
          });
    }

    [[nodiscard]] bool mayReplaceOther(
        const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<FixedPointConvertingRawDecorator<TargetUserType> const>(other);
      if(!casted) {
        return false;
      }
      if(_registerInfo != casted->_registerInfo) {
        return false;
      }

      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    NumericAddressedRegisterInfo _registerInfo;
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(
        FixedPointConvertingRawDecorator<TargetUserType>, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(
        FixedPointConvertingRawDecorator<TargetUserType>, setAsCooked_impl, 3);

    using NDRegisterAccessorDecorator<TargetUserType>::_target;
    using NDRegisterAccessor<TargetUserType>::buffer_2D;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
