// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendMuxedRegisterAccessor.h"

namespace ChimeraTK {
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendMuxedRegisterAccessor, FixedPointConverter);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendMuxedRegisterAccessor, IEEE754_SingleConverter);
} // namespace ChimeraTK
