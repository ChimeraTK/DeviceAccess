// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "createDataConverter.h"

#include "NumericAddressedRegisterCatalogue.h"

namespace ChimeraTK::detail {

  template<>
  FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> createDataConverter<
      FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>>(
      const NumericAddressedRegisterInfo& registerInfo, size_t channelIndex) {
    return FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerInfo.pathName,
        registerInfo.channels[channelIndex].width, registerInfo.channels[channelIndex].nFractionalBits,
        registerInfo.channels[channelIndex].signedFlag);
  }

  template<>
  IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(
      const NumericAddressedRegisterInfo&, [[maybe_unused]] size_t channelIndex) {
    return IEEE754_SingleConverter();
  }

} // namespace ChimeraTK::detail
