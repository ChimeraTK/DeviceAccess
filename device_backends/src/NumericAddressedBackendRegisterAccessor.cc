/*
 * NumericAddressedBackendRegisterAccessor.cc
 *
 *  Created on: Aug 12, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackendRegisterAccessor.h"
#if 0
namespace ChimeraTK {

  namespace detail {
    template<>
    FixedPointConverter createDataConverter<FixedPointConverter>(
        boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo) {
      return FixedPointConverter(
          registerInfo->name, registerInfo->width, registerInfo->nFractionalBits, registerInfo->signedFlag);
    }

    template<>
    IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(
        boost::shared_ptr<RegisterInfoMap::RegisterInfo> /*registerInfo*/) {
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
#endif //0
