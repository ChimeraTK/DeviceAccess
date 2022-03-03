/*
 * NumericAddressedBackendRegisterAccessor.cc
 *
 *  Created on: Aug 12, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  namespace detail {
    template<>
    FixedPointConverter createDataConverter<FixedPointConverter>(const NumericAddressedRegisterInfo& registerInfo) {
      return FixedPointConverter(registerInfo.pathName, registerInfo.channels.front().width,
          registerInfo.channels.front().nFractionalBits, registerInfo.channels.front().signedFlag);
    }

    template<>
    IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(const NumericAddressedRegisterInfo&) {
      return IEEE754_SingleConverter();
    }

  } // namespace detail

  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, FixedPointConverter, true);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, FixedPointConverter, false);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, true);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, false);

} /* namespace ChimeraTK */
