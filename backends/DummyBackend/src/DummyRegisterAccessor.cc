// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DummyRegisterAccessor.h"

namespace ChimeraTK::proxies {

  /********************************************************************************************************************/

  template<typename UserType, typename RawConverter>
  class RawConverterCapsuleImpl : public RawConverterCapsule<UserType> {
   public:
    explicit RawConverterCapsuleImpl(RawConverter converter) : _converter(converter) {}

    UserType toCooked(std::byte* raw) override;

    void toRaw(UserType cooked, std::byte* raw) override;

   private:
    RawConverter _converter;
  };

  /********************************************************************************************************************/

  template<typename UserType, typename RawConverter>
  UserType RawConverterCapsuleImpl<UserType, RawConverter>::toCooked(std::byte* raw) {
    using RawType = RawConverter::raw_type;
    RawType rawValue;
    std::memcpy(&rawValue, raw, sizeof(RawType));
    return _converter.toCooked(rawValue);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawConverter>
  void RawConverterCapsuleImpl<UserType, RawConverter>::toRaw(UserType cooked, std::byte* raw) {
    using RawType = RawConverter::raw_type;
    RawType rawValue = _converter.toRaw(cooked);
    std::memcpy(raw, &rawValue, sizeof(RawType));
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  std::shared_ptr<RawConverterCapsule<UserType>> RawConverterCapsule<UserType>::makeCapsule(
      const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex) {
    std::shared_ptr<RawConverterCapsule<UserType>> rv;

    callForRawType(info.channels[channelIndex].getRawType(), [&]([[maybe_unused]] auto dummy) {
      using RawType = std::make_unsigned_t<decltype(dummy)>;
      ChimeraTK::RawConverter::withConverter<UserType, RawType>(info, channelIndex, [&](auto converter) {
        rv = std::make_shared<RawConverterCapsuleImpl<UserType, decltype(converter)>>(converter);
      });
    });

    return rv;
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES_NO_VOID(RawConverterCapsule);

  /********************************************************************************************************************/

} // namespace ChimeraTK::proxies
