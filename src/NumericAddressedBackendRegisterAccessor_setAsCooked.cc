// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<typename COOKED_TYPE>
  void NumericAddressedBackendRegisterAccessor<UserType, isRaw>::setAsCooked_impl(
      unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    if constexpr(isRaw && std::is_integral_v<UserType>) {
      if constexpr(isRawType<std::make_unsigned_t<UserType>>) {
        // Note: the "UserType" is our RawType here!
        RawConverter::withConverter<COOKED_TYPE, std::make_unsigned_t<UserType>>(_registerInfo, 0, [&](auto converter) {
          NDRegisterAccessor<UserType>::buffer_2D[channel][sample] = UserType(converter.toRaw(value));
        });
      }
      return;
    }
    throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
  }

  /********************************************************************************************************************/

  namespace instantiator {
    void instantiateNumericAddressedBackendRegisterAccessorSetAsCooked(
        DataType typeU, DataType typeC, TransferElement* te) {
      callForType(typeU, [&]([[maybe_unused]] auto dummyU) {
        using UserType = decltype(dummyU);
        callForType(typeC, [&]([[maybe_unused]] auto dummyC) {
          using COOKED_TYPE = decltype(dummyC);
          auto naraCooked = dynamic_cast<NumericAddressedBackendRegisterAccessor<UserType, false>*>(te);
          naraCooked->setAsCooked_impl(0, 0, COOKED_TYPE{});
          auto naraRaw = dynamic_cast<NumericAddressedBackendRegisterAccessor<UserType, true>*>(te);
          naraRaw->setAsCooked_impl(0, 0, COOKED_TYPE{});
        });
      });
    };
  } // namespace instantiator

} // namespace ChimeraTK
