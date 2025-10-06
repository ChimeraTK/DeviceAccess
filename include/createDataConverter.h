// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "FixedPointConverter.h"
#include "IEEE754_SingleConverter.h"
#include "NumericAddressedRegisterCatalogue.h"

namespace ChimeraTK::detail {

  /** This function is external to allow template specialisation. */
  template<typename ConverterT>
  ConverterT createDataConverter(const NumericAddressedRegisterInfo& registerInfo, size_t channelIndex = 0);

  template<>
  FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> createDataConverter<
      FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>>(
      const NumericAddressedRegisterInfo& registerInfo, size_t channelIndex);

  template<>
  IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(
      const NumericAddressedRegisterInfo& registerInfo, size_t channelIndex);

} // namespace ChimeraTK::detail
