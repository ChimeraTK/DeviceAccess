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
        NumericAddressedRegisterInfo const& registerInfo);

    template<typename COOKED_TYPE>
    // NOLINTNEXTLINE(readability-identifier-naming)
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample);

    template<typename COOKED_TYPE>
    // NOLINTNEXTLINE(readability-identifier-naming)
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value);

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override;

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

  DECLARE_TEMPLATE_FOR_CHIMERATK_RAW_TYPES(FixedPointConvertingRawDecorator);

  // FIXME: get rid of the deprecated signed raw
  extern template class FixedPointConvertingRawDecorator<int8_t>;
  extern template class FixedPointConvertingRawDecorator<int16_t>;
  extern template class FixedPointConvertingRawDecorator<int32_t>;
  extern template class FixedPointConvertingRawDecorator<int64_t>;

} // namespace ChimeraTK
