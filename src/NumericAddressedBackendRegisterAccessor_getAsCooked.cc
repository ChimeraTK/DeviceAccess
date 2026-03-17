// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<typename COOKED_TYPE>
  COOKED_TYPE NumericAddressedBackendRegisterAccessor<UserType, isRaw>::getAsCooked_impl(
      unsigned int channel, unsigned int sample) {
    if constexpr(isRaw && std::is_integral_v<UserType>) {
      if constexpr(isRawType<std::make_unsigned_t<UserType>>) {
        // Note: the "UserType" is our RawType here!
        COOKED_TYPE rv;
        RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<UserType>>(_registerInfo, 0, [&](auto converter) {
          rv = converter.toCooked(
              std::make_unsigned_t<UserType>(NDRegisterAccessor<UserType>::buffer_2D[channel][sample]));
        });
        return rv;
      }
    }
    throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
  }

  /********************************************************************************************************************/

  namespace instantiator {
    void instantiateNumericAddressedBackendRegisterAccessorGetAsCooked(
        DataType typeU, DataType typeC, TransferElement* te) {
      callForType(typeU, [&]([[maybe_unused]] auto dummyU) {
        using UserType = decltype(dummyU);
        callForType(typeC, [&]([[maybe_unused]] auto dummyC) {
          using COOKED_TYPE = decltype(dummyC);
          auto naraCooked = dynamic_cast<NumericAddressedBackendRegisterAccessor<UserType, false>*>(te);
          naraCooked->template getAsCooked_impl<COOKED_TYPE>(0, 0);
          auto naraRaw = dynamic_cast<NumericAddressedBackendRegisterAccessor<UserType, true>*>(te);
          naraRaw->template getAsCooked_impl<COOKED_TYPE>(0, 0);
        });
      });
    };
  } // namespace instantiator

} // namespace ChimeraTK
