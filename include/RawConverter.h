// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedRegisterCatalogue.h"

#include <cstdint>
#include <tuple>

namespace ChimeraTK::RawConverter {

  /********************************************************************************************************************/

  /**
   * Create ToCooked converter object for the given ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo and pass it to
   * the given functor (typically a lambda).
   *
   * Place the loop over the elements to convert inside the lambda to optimise performance.
   */
  template<typename UserType, typename F>
  void withToCooked(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun);

  /********************************************************************************************************************/

  /**
   * Create ToCooked converter object for the given ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo and pass it
   * to the given functor (typically a lambda).
   *
   * Place the loop over the elements to convert inside the lambda to optimise performance.
   */
  template<typename UserType, typename F>
  void withToRaw(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun);

  /********************************************************************************************************************/

  enum class SignificantBitsCase {
    // Note: values must match index in SignificantBitsCaseTypes tuple
    bit8 = 0,
    bit16,
    bit32,
    bit64,
    generic // Generic must be last, so we can loop
  };

  /********************************************************************************************************************/

  enum class FractionalCase { integer, fixedPositive, fixedNegative, ieee754_32 };

  /********************************************************************************************************************/

  /**
   * Converter class for conversions from raw to cooked values. Users shall use the
   * ChimeraTK::RawConverter::withToCooked() function to create such object.
   */
  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  class ToCooked {
   public:
    explicit ToCooked(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info);

    UserType convert(RawType rawValue);

   private:
    RawType _signBitMask, _usedBitMask, _unusedBitMask;

    // The PromotedRawType has the same width as the RawType but the signedness according to isSigned. We will store the
    // "promoted" raw value in it, i.e. the arbitrary bit width of the raw has been changed into a proper CPU data type.
    using PromotedRawType = std::conditional_t<isSigned, std::make_signed_t<RawType>, RawType>;

    // Use double as intermediate conversion target, unless user has requested float (to avoid unnecessary conversion
    // to float via double).
    using FloatIntermediate = std::conditional_t<std::is_same_v<UserType, float>, float, double>;

    // We only want to store a conversion factor, if we need it. Otherwise we use ChimeraTK::Void as an empty type which
    // in combination with [[no_unique_address]] will have even no memory footprint.
    using FloatIntermediateConditional =
        std::conditional_t<fc != FractionalCase::integer, FloatIntermediate, ChimeraTK::Void>;

    [[no_unique_address]] FloatIntermediateConditional _conversionFactor;

    // Also store the fractional bits only if needed. We need only the negative case, so we store it with the opposite
    // sign to have a positive value (needed for the bit shift)
    [[no_unique_address]] std::conditional_t<fc == FractionalCase::fixedNegative, uint32_t, ChimeraTK::Void>
        _nNegativeFractionalBits;
  };

  /********************************************************************************************************************/

  /**
   * Converter class for conversions from cooked to raw values. Users shall use the
   * ChimeraTK::RawConverter::withToRaw() function to create such object.
   */
  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  class ToRaw {
   public:
    explicit ToRaw(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info);

    RawType convert(UserType cookedValue);

   private:
    RawType _signBitMask, _usedBitMask, _unusedBitMask;
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /* Note: Only implementations and helper classes/functions below this point  */

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  namespace detail {

    /******************************************************************************************************************/

    // Note: order must match SignificantBitsCase enum
    using SignificantBitsCaseTypes = std::tuple<int8_t, int16_t, int32_t, int64_t>;
    // constexpr SignificantBitsCaseTypes _significantBitsCaseTypes;

    /******************************************************************************************************************/

    // // Helper function to get the enum value from a given type
    // template<typename T, typename... Types>
    // constexpr SignificantBitsCase significantBitsCaseForType(
    //     [[maybe_unused]] std::tuple<Types...> _ = _significantBitsCaseTypes) {
    //   constexpr auto matches = {std::is_same_v<T, Types>...};

    //   for(std::size_t i = 0; i < sizeof...(Types); ++i) {
    //     if(matches[i]) {
    //       return SignificantBitsCase(i);
    //     }
    //   }
    //   return SignificantBitsCase::generic;
    // }

    /******************************************************************************************************************/

    // // TODO: Move to some central place (also used in testNumericConverter)
    // template<typename Tuple, typename F>
    // void forEachType(F&& f) {
    //   []<std::size_t... Is>(F&& g, std::index_sequence<Is...>) {
    //     (g.template operator()<std::tuple_element_t<Is, Tuple>>(), ...);
    //   }(std::forward<F>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    // }

    /******************************************************************************************************************/

    template<typename F>
    void callForSignificantBitsCase(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
      switch(info.nFractionalBits) {
        case 8:
          std::forward<F>(fun).template operator()<SignificantBitsCase::bit8>();
          return;
        case 16:
          std::forward<F>(fun).template operator()<SignificantBitsCase::bit16>();
          return;
        case 32:
          std::forward<F>(fun).template operator()<SignificantBitsCase::bit32>();
          return;
        case 64:
          std::forward<F>(fun).template operator()<SignificantBitsCase::bit64>();
          return;
        default:
          std::forward<F>(fun).template operator()<SignificantBitsCase::generic>();
          return;
      }
    }

    /******************************************************************************************************************/

    template<typename F>
    void callForFractionalCase(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
      if(info.nFractionalBits == 0) {
        std::forward<F>(fun).template operator()<FractionalCase::integer>();
      }
      else if(info.nFractionalBits > 0) {
        std::forward<F>(fun).template operator()<FractionalCase::fixedPositive>();
      }
      else if(info.nFractionalBits < 0) {
        std::forward<F>(fun).template operator()<FractionalCase::fixedNegative>();
      }
    }

    /******************************************************************************************************************/

    template<typename F>
    void callWithConverterParams(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
      // get RawType from info
      callForRawType(info.getRawType(), [&](auto x) {
        using RawType = std::make_unsigned_t<decltype(x)>;
        // get number of bits from info and determine SignificantBitsCase
        detail::callForSignificantBitsCase(info, [&]<SignificantBitsCase significantBitsCase> {
          // get number of fractional bits from info and determine FractionalCase
          detail::callForFractionalCase(info, [&]<FractionalCase fractionalCase> {
            // distinguish signed/unsigned and do the call
            if(info.signedFlag) {
              std::forward<F>(fun).template operator()<RawType, significantBitsCase, fractionalCase, true>();
            }
            else {
              std::forward<F>(fun).template operator()<RawType, significantBitsCase, fractionalCase, false>();
            }
          });
        });
      });
    }

    /******************************************************************************************************************/

    template<std::size_t... Is>
    constexpr auto makeUnusedBitMaskTable(std::index_sequence<Is...>) {
      return std::array<uint64_t, 64>{(~uint64_t{} << Is)...};
    }

    /******************************************************************************************************************/

    template<std::size_t... Is>
    constexpr auto makeUsedBitMaskTable(std::index_sequence<Is...>) {
      return std::array<uint64_t, 64>{(~(~uint64_t{} << Is))...};
    }

    /******************************************************************************************************************/

    template<std::size_t... Is>
    constexpr auto makeSignBitMaskTable(std::index_sequence<Is...>) {
      return std::array<uint64_t, 64>{(Is > 0 ? (uint64_t{1} << (Is - 1)) : 0)...};
    }

    /******************************************************************************************************************/

    constexpr auto unusedBitMaskTable = makeUnusedBitMaskTable(std::make_index_sequence<64>{});
    constexpr auto usedBitMaskTable = makeUsedBitMaskTable(std::make_index_sequence<64>{});
    constexpr auto signBitMaskTable = makeSignBitMaskTable(std::make_index_sequence<64>{});

    // returns the raw value "promoted" to the full RawType
    template<typename PromotedRawType, typename RawType, SignificantBitsCase significantBitsCase>
    constexpr RawType interpretArbitraryBitInteger(
        RawType signBitMask, RawType usedBitMask, RawType unusedBitMask, RawType rawValue) {
      static_assert(std::is_integral_v<RawType>);
      static_assert(!std::is_signed_v<RawType>);

      if constexpr(significantBitsCase == SignificantBitsCase::generic) {
        if constexpr(std::is_signed_v<PromotedRawType>) {
          if(!(rawValue & signBitMask)) { // sign bit not set: force unused bits to zero
            return PromotedRawType(rawValue & usedBitMask);
          }
          // sign bit set: force unused bits to one
          return PromotedRawType(rawValue | unusedBitMask);
        }
        else {
          // unsigned value: force unused bits to zero
          return PromotedRawType(rawValue & usedBitMask);
        }
      }
      else {
        using SignedType = std::tuple_element_t<int(significantBitsCase), SignificantBitsCaseTypes>;
        if constexpr(std::is_signed_v<PromotedRawType>) {
          return PromotedRawType(SignedType(rawValue));
        }
        else {
          return PromotedRawType(std::make_unsigned_t<SignedType>(rawValue));
        }
      }
    }

    /******************************************************************************************************************/

  } // namespace detail

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType, typename F>
  void withToCooked(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
    detail::callWithConverterParams(
        info, [&]<typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned> {
          ToCooked<UserType, RawType, sc, fc, isSigned> converter(info);
          std::forward<F>(fun)(converter);
        });
  }

  /********************************************************************************************************************/

  /**
   * Create ToCooked converter object for the given ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo and pass it
   * to the given functor (typically a lambda).
   *
   * Place the loop over the elements to convert inside the lambda to optimise performance.
   */
  template<typename UserType, typename F>
  void withToRaw(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
    callWithConverterParams(info, [&]<typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned> {
      ToRaw<UserType, RawType, sc, fc, isSigned> converter(info);
      std::forward<F>(fun)(converter);
    });
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  ToCooked<UserType, RawType, sc, fc, isSigned>::ToCooked(
      const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info)
  : _signBitMask(detail::signBitMaskTable[info.width]), _usedBitMask(detail::usedBitMaskTable[info.width]),
    _unusedBitMask(detail::unusedBitMaskTable[info.width]) {
    if constexpr(fc != FractionalCase::integer) {
      _conversionFactor =
          FloatIntermediateConditional(1.) / std::pow(FloatIntermediateConditional(2), info.nFractionalBits);
    }
    if constexpr(fc == FractionalCase::fixedNegative) {
      _nNegativeFractionalBits = -info.nFractionalBits;
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  UserType ToCooked<UserType, RawType, sc, fc, isSigned>::convert(RawType rawValue) {
    auto promotedRawValue = detail::interpretArbitraryBitInteger<PromotedRawType, RawType, sc>(
        _signBitMask, _usedBitMask, _unusedBitMask, rawValue);

    if constexpr(fc == FractionalCase::integer) {
      return numericToUserType<UserType>(promotedRawValue);
    }
    else if constexpr(fc == FractionalCase::fixedPositive ||
        (fc == FractionalCase::fixedNegative && std::is_floating_point_v<UserType>)) {
      // Positive fractional bits will always convert through an intermediate float, to have proper rounding.
      // Negative fractional bits can use the same code path efficiently, if the UserType is floating point.
      static_assert(std::is_same_v<FloatIntermediate, FloatIntermediateConditional>);
      return numericToUserType<UserType>(FloatIntermediate(promotedRawValue) * _conversionFactor);
    }
    else if constexpr(fc == FractionalCase::fixedNegative) {
      auto intermediate = uint64_t(promotedRawValue);
      intermediate <<= _nNegativeFractionalBits;
      return numericToUserType<UserType>(intermediate);
    }
    else {
      static_assert(fc == FractionalCase::ieee754_32);
      return numericToUserType<UserType>(std::bit_cast<float>(promotedRawValue));
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::RawConverter
