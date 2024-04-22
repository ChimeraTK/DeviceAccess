// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, FixedPointConverter, true);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, FixedPointConverter, false);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, true);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, false);

} /* namespace ChimeraTK */
