// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceConvertingRawDecorator.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename TargetUserType>
  ConvertingRawDecorator<TargetUserType>::ConvertingRawDecorator(
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
      NumericAddressedRegisterInfo const& registerInfo)
  : NDRegisterAccessorDecorator<TargetUserType>(target), _registerInfo(registerInfo) {
    static_assert(isRawType<std::make_unsigned_t<TargetUserType>>);
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
  }

  /********************************************************************************************************************/

  template<typename TargetUserType>
  template<typename COOKED_TYPE>
  // NOLINTNEXTLINE(readability-identifier-naming)
  COOKED_TYPE ConvertingRawDecorator<TargetUserType>::getAsCooked_impl(unsigned int channel, unsigned int sample) {
    COOKED_TYPE rv;
    RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<TargetUserType>>(
        _registerInfo, 0, [&](auto converter) {
          rv = converter.toCooked(
              std::make_unsigned_t<TargetUserType>(NDRegisterAccessor<TargetUserType>::buffer_2D[channel][sample]));
        });
    return rv;
  }

  /********************************************************************************************************************/

  template<typename TargetUserType>
  template<typename COOKED_TYPE>
  // NOLINTNEXTLINE(readability-identifier-naming)
  void ConvertingRawDecorator<TargetUserType>::setAsCooked_impl(
      unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<TargetUserType>>(
        _registerInfo, 0, [&](auto converter) {
          NDRegisterAccessor<TargetUserType>::buffer_2D[channel][sample] = TargetUserType(converter.toRaw(value));
        });
  }

  /********************************************************************************************************************/

  template<typename TargetUserType>
  [[nodiscard]] bool ConvertingRawDecorator<TargetUserType>::mayReplaceOther(
      const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const {
    auto casted = boost::dynamic_pointer_cast<ConvertingRawDecorator<TargetUserType> const>(other);
    if(!casted) {
      return false;
    }
    if(_registerInfo != casted->_registerInfo) {
      return false;
    }

    return _target->mayReplaceOther(casted->_target);
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_RAW_TYPES(ConvertingRawDecorator);

  // FIXME: get rid of the deprecated signed raw
  template class ConvertingRawDecorator<int8_t>;
  template class ConvertingRawDecorator<int16_t>;
  template class ConvertingRawDecorator<int32_t>;
  template class ConvertingRawDecorator<int64_t>;

} // namespace ChimeraTK
