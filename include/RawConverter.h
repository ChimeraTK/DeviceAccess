// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedRegisterCatalogue.h"

#include <cstdint>
#include <tuple>

namespace ChimeraTK::RawConverter {

  /********************************************************************************************************************/

  // Note: values must match index in SignificantBitsCaseTypes tuple
  enum class SignificantBitsCase { bit8 = 0, bit16, bit32, bit64, generic };

  /********************************************************************************************************************/

  enum class FractionalCase { integer, fixedPositive, fixedNegative, fixedNegativeFast, ieee754_32 };

  /********************************************************************************************************************/

  /**
   * Converter class for conversions from raw to cooked values.
   *
   * For maximum performance, some template parameters are added so generate optimised code branches for various
   * conversion cases. This allows us to implement some decisions in critical pathes as "if constexpr", at the expense
   * of a more complicated construction of the converter object.
   *
   * The accessors which use the Converter shall not be required to be templated on the exact converter type (which
   * would not work in case of the 2D multiplexed accessor at all). Hence the ConverterLoopHelper class is introduced to
   * erase this type through a virtual base class. This allows to write the loop code in the accessor implementation
   * (which may need to be optimised in a particular way as well), while having the (non-virtual) Converter object
   * available for conversion.
   *
   * For implementing getAsCooked/setAsCooked (which are templated on the COOKED_TYPE and hence require a new converter
   * instance on each call), the function withConverter() can be used.
   */
  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  class Converter {
   public:
    explicit Converter(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info);

    UserType toCooked(RawType rawValue);

    RawType toRaw(UserType cookedValue);

   private:
    RawType _signBitMask, _usedBitMask, _unusedBitMask;
    UserType _minCookedValues, _maxCookedValues;
    RawType _minRawValue, _maxRawValue;

    // The PromotedRawType has the same width as the RawType but the signedness according to isSigned. We will store the
    // "promoted" raw value in it, i.e. the arbitrary bit width of the raw has been changed into a proper CPU data type.
    using PromotedRawType = std::conditional_t<isSigned, std::make_signed_t<RawType>, RawType>;

    // Use double as intermediate conversion target, unless user has requested float (to avoid unnecessary conversion
    // to float via double).
    using FloatIntermediate = std::conditional_t<std::is_same_v<UserType, float>, float, double>;

    FloatIntermediate _conversionFactor, _inverseConversionFactor;

    // We need the negative fractional bits as a positive value (needed for the bit shift). This field is unused unless
    // fc == FractionalCase::fixedNegative or fixedNegativeFast.
    uint32_t _nNegativeFractionalBits;
  };

  /********************************************************************************************************************/

  /**
   * Abstract base class to implement erasure of the exact Converter type.
   */
  class ConverterLoopHelper {
   public:
    ConverterLoopHelper(const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex);

    virtual ~ConverterLoopHelper() = default;

    /**
     * Call doPostReadImpl on the accessor passed to makeConverterLoopHelper().
     *
     * The doPostReadImpl function of the Accessor must have the following signature:
     *
     *  template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc,
     *           RawConverter::FractionalCase fc, bool isSigned>
     *  void MyAccessor::doPostReadImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
     *                                  size_t channelIndex);
     *
     * Please note, that the accessor might be already templated on the UserType, which will be always identical to
     * the CookedType. We keep this in, since in case of the 2D multiplexed accessor we might have a different
     * CookedType for each channel.
     */
    virtual void doPostRead() = 0;

    /**
     * Call doPreWriteImpl on the accessor passed to makeConverterLoopHelper().
     *
     * The doPreWriteImpl function of the Accessor must have the same signature as doPostReadImpl.
     */
    virtual void doPreWrite() = 0;

    /**
     * Create ConverterLoopHelper with the Converter object matching the given RegisterInfo and channel. On the given
     * accessor, the ConverterLoopHelper will call doPostReadImpl/doPreWriteImpl when doPostRead/doPreWrite is called.
     */
    template<typename UserType, typename Accessor>
    static std::unique_ptr<ConverterLoopHelper> makeConverterLoopHelper(
        const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex, Accessor& accessor);

   protected:
    const ChimeraTK::NumericAddressedRegisterInfo& _info;
    const size_t _channelIndex;
  };

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned,
      typename Accessor>
  class ConverterLoopHelperImpl : public ConverterLoopHelper {
   public:
    ConverterLoopHelperImpl(const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex,
        Converter<UserType, RawType, sc, fc, isSigned> converter, Accessor& accessor);

    void doPostRead() override;

    void doPreWrite() override;

   private:
    Converter<UserType, RawType, sc, fc, isSigned> _converter;
    Accessor& _accessor;
  };

  /********************************************************************************************************************/

  /**
   * Create Converter matching the given register info and channel, and call the functor object passing the Converter
   * object. This can be used to implement setAsCooked/getAsCooked in the accessors. For the converter used in
   * postRead/preWrite, it is recommended to use the ConverterLoopHelper instead.
   */
  template<typename UserType, typename RawType, typename FUN>
  void withConverter(const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex, FUN&& fun);

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /* Note: Only implementations and helper classes/functions below this point  */

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  namespace detail {

    /******************************************************************************************************************/

    // Note: order must match SignificantBitsCase enum
    using SignificantBitsCaseTypes = std::tuple<uint8_t, uint16_t, uint32_t, uint64_t>;

    /******************************************************************************************************************/

    constexpr int getMinWidthsForFractionalCase(const FractionalCase& fc) {
      switch(fc) {
        case FractionalCase::ieee754_32:
          return 32;
        default:
          return 8;
      }
    }

    /******************************************************************************************************************/

    template<typename RawType, typename F>
    void callForSignificantBitsCase(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
      switch(info.width) {
        case 8:
          if constexpr(std::numeric_limits<RawType>::digits >= 8) {
            std::forward<F>(fun).template operator()<SignificantBitsCase::bit8>();
            return;
          }
        case 16:
          if constexpr(std::numeric_limits<RawType>::digits >= 16) {
            std::forward<F>(fun).template operator()<SignificantBitsCase::bit16>();
            return;
          }
        case 32:
          if constexpr(std::numeric_limits<RawType>::digits >= 32) {
            std::forward<F>(fun).template operator()<SignificantBitsCase::bit32>();
            return;
          }
        case 64:
          if constexpr(std::numeric_limits<RawType>::digits >= 64) {
            std::forward<F>(fun).template operator()<SignificantBitsCase::bit64>();
            return;
          }
      }
      std::forward<F>(fun).template operator()<SignificantBitsCase::generic>();
    }

    /******************************************************************************************************************/

    template<typename RawType, typename F>
    void callForFractionalCase(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info, F&& fun) {
      switch(info.dataType) {
        case NumericAddressedRegisterInfo::Type::FIXED_POINT:
          if(info.nFractionalBits == 0) {
            std::forward<F>(fun).template operator()<FractionalCase::integer>();
            return;
          }
          else if(info.nFractionalBits > 0) {
            std::forward<F>(fun).template operator()<FractionalCase::fixedPositive>();
            return;
          }
          else if(info.nFractionalBits < 0) {
            if(std::numeric_limits<RawType>::digits >= info.width - info.nFractionalBits) {
              // We can make an even faster conversion if the bit shifted raw value still fits into the RawType.
              std::forward<F>(fun).template operator()<FractionalCase::fixedNegativeFast>();
              return;
            }
            std::forward<F>(fun).template operator()<FractionalCase::fixedNegative>();
            return;
          }
          break;
        case NumericAddressedRegisterInfo::Type::IEEE754:
          if constexpr(std::numeric_limits<RawType>::digits >= 32) {
            std::forward<F>(fun).template operator()<FractionalCase::ieee754_32>();
            return;
          }
          break;
        case NumericAddressedRegisterInfo::Type::VOID:
        case NumericAddressedRegisterInfo::Type::ASCII:
          break;
      }
      throw std::logic_error("NOT IMPLEMENTED");
    }

    /******************************************************************************************************************/

    template<typename UserType, typename RawType, typename F>
    void callWithConverterParamsFixedRaw(
        const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex, F&& fun) {
      // get number of bits from info and determine SignificantBitsCase
      detail::callForSignificantBitsCase<RawType>(info.channels[channelIndex], [&]<SignificantBitsCase sc> {
        // get number of fractional bits from info and determine FractionalCase
        detail::callForFractionalCase<RawType>(info.channels[channelIndex], [&]<FractionalCase fc> {
          if constexpr(std::numeric_limits<RawType>::digits >= detail::getMinWidthsForFractionalCase(fc)) {
            if constexpr(fc == FractionalCase::ieee754_32) {
              // special case: IEEE754 is always signed, so we can avoid an additional code instance
              std::forward<F>(fun).template operator()<RawType, sc, fc, true>();
            }
            else {
              // Fractional/Integers: distinguish signed/unsigned and do the call
              if(info.channels[channelIndex].signedFlag) {
                std::forward<F>(fun).template operator()<RawType, sc, fc, true>();
              }
              else {
                std::forward<F>(fun).template operator()<RawType, sc, fc, false>();
              }
            }
          }
          else {
            throw ChimeraTK::logic_error(
                std::format("Requested data type does not fit into the raw data width for register '{}', channel {}.",
                    std::string(info.getRegisterName()), channelIndex));
          }
        });
      });
    }

    /******************************************************************************************************************/

    template<typename UserType, typename F>
    void callWithConverterParams(const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex, F&& fun) {
      // Special case for FundamentalType::nodata, i.e. RawType = ChimeraTK::Void. Use full template specialisation.
      // This special case saves us a couple of unnecessary code instantiations and hence speeds up compile time.
      if(info.dataDescriptor.fundamentalType() == DataDescriptor::FundamentalType::nodata) {
        std::forward<F>(fun)
            .template operator()<ChimeraTK::Void, SignificantBitsCase::generic, FractionalCase::integer, false>();
        return;
      }

      // get RawType from info
      callForRawType(info.channels[channelIndex].getRawType(), [&](auto x) {
        using RawType = std::make_unsigned_t<decltype(x)>;
        if constexpr(std::is_same_v<UserType, ChimeraTK::Void>) {
          // Special case for UserType = ChimeraTK::Void. Use full template specialisation. We do not care about
          // details in the conversion, since there is only one possible value. This special case saves us a couple of
          // unnecessary code instantiations and hence speeds up compile time.
          std::forward<F>(fun)
              .template operator()<RawType, SignificantBitsCase::generic, FractionalCase::integer, false>();
        }
        else {
          // Regular case for non-void UserTypes
          callWithConverterParamsFixedRaw<UserType, RawType>(info, channelIndex, fun);
        }
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

    /******************************************************************************************************************/

    // returns the raw value "promoted" to the full RawType
    template<typename PromotedRawType, typename RawType, SignificantBitsCase significantBitsCase>
    constexpr PromotedRawType interpretArbitraryBitInteger(
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
        // Here we make use of the ability of the CPU to actually understand a type with the given number of bits, so we
        // can promote it to our PromotedRawType with standard cast operations. Keep in mind that this has nothing to do
        // with the width of the RawType. Example: RawType is uint32_t but we have 8 significant bits (including sign
        // bit). We will cast the raw value into (u)int8_t (depending on the selected signedness) and then into the
        // PromotedRawType. This cuts away any extra bits and properly takes care of the sign.
        using UnsignedType = std::tuple_element_t<int(significantBitsCase), SignificantBitsCaseTypes>;
        using SignedType = std::make_signed_t<UnsignedType>;
        if constexpr(std::is_signed_v<PromotedRawType>) {
          return PromotedRawType(SignedType(rawValue));
        }
        else {
          return PromotedRawType(UnsignedType(rawValue));
        }
      }
    }

    /******************************************************************************************************************/

  } // namespace detail

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  Converter<UserType, RawType, sc, fc, isSigned>::Converter(
      const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo& info)
  : _signBitMask(isSigned ? detail::signBitMaskTable[info.width] : 0),
    _usedBitMask(detail::usedBitMaskTable[info.width]), _unusedBitMask(detail::unusedBitMaskTable[info.width]) {
    // sanity check for parameters
    if constexpr(fc == FractionalCase::fixedPositive || fc == FractionalCase::fixedNegative ||
        fc == FractionalCase::fixedNegativeFast) {
      constexpr uint32_t maxRawWidth = std::numeric_limits<uint64_t>::digits;
      if(info.width > maxRawWidth) {
        throw ChimeraTK::logic_error(
            std::format("RawConverter cannot deal with a bit width of {} > {}.", info.width, maxRawWidth));
      }
      if(info.nFractionalBits > int32_t(info.width)) {
        throw ChimeraTK::logic_error(std::format(
            "RawConverter cannot deal with {} fractional bits (larger than total width).", info.nFractionalBits));
      }
      if(info.nFractionalBits < -int32_t(maxRawWidth - info.width)) {
        throw ChimeraTK::logic_error(std::format(
            "RawConverter cannot deal with {} fractional bits (too negative, result doesn't fit in {} bits).",
            info.nFractionalBits, maxRawWidth));
      }
    }
    // Fixed point conversion uses either floating-point conversion factors or bit shift
    if constexpr(fc == FractionalCase::fixedPositive || fc == FractionalCase::fixedNegative ||
        fc == FractionalCase::fixedNegativeFast) {
      // This is used for toRaw conversions, where we never do bit shifts to get proper rounding
      _inverseConversionFactor = std::pow(FloatIntermediate(2), info.nFractionalBits);
    }
    if constexpr(fc == FractionalCase::fixedPositive ||
        ((fc == FractionalCase::fixedNegative || fc == FractionalCase::fixedNegativeFast) &&
            std::is_floating_point_v<UserType>)) {
      // This is used for toCooked conversions when floating point is involved
      _conversionFactor = FloatIntermediate(1.) / _inverseConversionFactor;
    }
    else if constexpr(fc == FractionalCase::fixedNegative || fc == FractionalCase::fixedNegativeFast) {
      // This is used for toCooked conversions when we can to a bit shift (int-to-int with negative fractional bits)
      _nNegativeFractionalBits = -info.nFractionalBits;
    }

    // toRaw conversion needs to know minimum and maximum value range for clamping
    // This must come last, since we already call toCooked (which does not need these values for clamping)
    if constexpr(fc == FractionalCase::fixedNegative || fc == FractionalCase::fixedNegativeFast ||
        fc == FractionalCase::fixedPositive || fc == FractionalCase::integer) {
      if constexpr(isSigned) {
        _maxRawValue = _usedBitMask ^ _signBitMask;
        _minRawValue = _signBitMask;
      }
      else {
        _maxRawValue = _usedBitMask;
        _minRawValue = 0;
      }
    }
    else {
      static_assert(fc == FractionalCase::ieee754_32);
      _maxRawValue = RawType(std::bit_cast<uint32_t>(std::numeric_limits<float>::max()));
      _minRawValue = RawType(std::bit_cast<uint32_t>(std::numeric_limits<float>::lowest()));
    }
    _maxCookedValues = toCooked(_maxRawValue);
    _minCookedValues = toCooked(_minRawValue);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  UserType Converter<UserType, RawType, sc, fc, isSigned>::toCooked(RawType rawValue) {
    if constexpr(fc == FractionalCase::integer) {
      auto promotedRawValue = detail::interpretArbitraryBitInteger<PromotedRawType, RawType, sc>(
          _signBitMask, _usedBitMask, _unusedBitMask, rawValue);
      return numericToUserType<UserType>(promotedRawValue);
    }
    else if constexpr(fc == FractionalCase::fixedPositive ||
        (fc == FractionalCase::fixedNegative && std::is_floating_point_v<UserType>)) {
      // Positive fractional bits will always convert through an intermediate float, to have proper rounding.
      // Negative fractional bits can use the same code path efficiently, if the UserType is floating point.
      auto promotedRawValue = detail::interpretArbitraryBitInteger<PromotedRawType, RawType, sc>(
          _signBitMask, _usedBitMask, _unusedBitMask, rawValue);
      return numericToUserType<UserType>(FloatIntermediate(promotedRawValue) * _conversionFactor);
    }
    else if constexpr(fc == FractionalCase::fixedNegative) {
      // In signed case, make sure to fill up the leading 1s if negative.
      // @TODO Write test for signed negative case!!
      using IntIntermediate = std::conditional_t<isSigned, int64_t, uint64_t>;
      auto promotedRawValue = detail::interpretArbitraryBitInteger<IntIntermediate, RawType, sc>(
          _signBitMask, _usedBitMask, _unusedBitMask, rawValue);
      auto intermediateUnsigned = uint64_t(promotedRawValue);
      intermediateUnsigned <<= _nNegativeFractionalBits;
      return numericToUserType<UserType>(IntIntermediate(intermediateUnsigned));
    }
    else if constexpr(fc == FractionalCase::fixedNegativeFast) {
      // This is the shortcut which avoids promoting to a 64 bit intermediate type. In this case, we know the bit
      // shifted raw value still fits into the RawType.
      auto promotedRawValue = detail::interpretArbitraryBitInteger<PromotedRawType, RawType, sc>(
          _signBitMask, _usedBitMask, _unusedBitMask, rawValue);
      auto intermediateUnsigned = RawType(promotedRawValue);
      intermediateUnsigned <<= _nNegativeFractionalBits;
      return numericToUserType<UserType>(PromotedRawType(intermediateUnsigned));
    }
    else {
      static_assert(fc == FractionalCase::ieee754_32);
      static_assert(std::numeric_limits<RawType>::digits >= 32);
      static_assert(std::numeric_limits<float>::is_iec559);
      auto promotedRawValue = detail::interpretArbitraryBitInteger<PromotedRawType, RawType, sc>(
          _signBitMask, _usedBitMask, _unusedBitMask, rawValue);
      return numericToUserType<UserType>(std::bit_cast<float>(uint32_t(promotedRawValue)));
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned>
  RawType Converter<UserType, RawType, sc, fc, isSigned>::toRaw(UserType cookedValue) {
    // Do a range check first. The later overflow check in the conversion is not
    // sufficient, since we can have non-standard word sizes like 12 bits.
    if constexpr(!std::is_same_v<UserType, ChimeraTK::Void>) {
      if(cookedValue < _minCookedValues) {
        return _minRawValue;
      }
      if(cookedValue > _maxCookedValues) {
        return _maxRawValue;
      }
    }

    PromotedRawType promotedRawValue;

    if constexpr(fc == FractionalCase::integer) {
      promotedRawValue = userTypeToNumeric<PromotedRawType>(cookedValue);
    }
    else if constexpr(fc == FractionalCase::fixedPositive || fc == FractionalCase::fixedNegative ||
        fc == FractionalCase::fixedNegativeFast) {
      // All fractional bit cases will always convert through an intermediate float, to have proper rounding.
      promotedRawValue = userTypeToNumeric<PromotedRawType>(
          userTypeToNumeric<FloatIntermediate>(cookedValue) * _inverseConversionFactor);
    }
    else {
      static_assert(fc == FractionalCase::ieee754_32);
      static_assert(std::numeric_limits<RawType>::digits >= 32);
      static_assert(std::numeric_limits<float>::is_iec559);
      promotedRawValue = PromotedRawType(std::bit_cast<uint32_t>(userTypeToNumeric<float>(cookedValue)));
    }

    return RawType(promotedRawValue) & _usedBitMask;
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType, typename Accessor>
  std::unique_ptr<ConverterLoopHelper> ConverterLoopHelper::makeConverterLoopHelper(
      const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex, Accessor& accessor) {
    std::unique_ptr<ConverterLoopHelper> rv;

    detail::callWithConverterParams<UserType>(
        info, channelIndex, [&]<typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned> {
          Converter<UserType, RawType, sc, fc, isSigned> converter(info.channels[channelIndex]);
          rv = std::make_unique<ConverterLoopHelperImpl<UserType, RawType, sc, fc, isSigned, Accessor>>(
              info, channelIndex, converter, accessor);
        });

    assert(rv != nullptr);
    return rv;
  }

  /********************************************************************************************************************/

  inline ConverterLoopHelper::ConverterLoopHelper(
      const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex)
  : _info(info), _channelIndex(channelIndex) {}

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned,
      typename Accessor>
  ConverterLoopHelperImpl<UserType, RawType, sc, fc, isSigned, Accessor>::ConverterLoopHelperImpl(
      const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex,
      Converter<UserType, RawType, sc, fc, isSigned> converter, Accessor& accessor)
  : ConverterLoopHelper(info, channelIndex), _converter(converter), _accessor(accessor) {}

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned,
      typename Accessor>
  void ConverterLoopHelperImpl<UserType, RawType, sc, fc, isSigned, Accessor>::doPostRead() {
    _accessor.template doPostReadImpl<UserType, RawType, sc, fc, isSigned>(_converter, _channelIndex);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, SignificantBitsCase sc, FractionalCase fc, bool isSigned,
      typename Accessor>
  void ConverterLoopHelperImpl<UserType, RawType, sc, fc, isSigned, Accessor>::doPreWrite() {
    _accessor.template doPreWriteImpl<UserType, RawType, sc, fc, isSigned>(_converter, _channelIndex);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawType, typename FUN>
  void withConverter(const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex, FUN&& fun) {
    detail::callWithConverterParamsFixedRaw<UserType, RawType>(
        info, channelIndex, [&]<typename RawTypeAgain, SignificantBitsCase sc, FractionalCase fc, bool isSigned> {
          static_assert(std::is_same_v<RawTypeAgain, RawType>);
          Converter<UserType, RawType, sc, fc, isSigned> converter(info.channels[channelIndex]);
          std::forward<FUN>(fun).operator()(converter);
        });
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /**
   * Specialisation for FundamentalType::nodata (RawType = Void) and/or UserType = Void
   */
  template<typename UserType, typename RawType>
    requires(std::is_same_v<UserType, ChimeraTK::Void> || std::is_same_v<RawType, ChimeraTK::Void>)
  class Converter<UserType, RawType, SignificantBitsCase::generic, FractionalCase::integer, false> {
   public:
    explicit Converter(const ChimeraTK::NumericAddressedRegisterInfo::ChannelInfo&) {}

    UserType toCooked(RawType rawValue);

    RawType toRaw(UserType cookedValue);
  };

  /********************************************************************************************************************/

  template<typename UserType, typename RawType>
    requires(std::is_same_v<UserType, ChimeraTK::Void> || std::is_same_v<RawType, ChimeraTK::Void>)
  UserType Converter<UserType, RawType, SignificantBitsCase::generic, FractionalCase::integer, false>::toCooked(
      RawType) {
    return UserType{};
  }

  /********************************************************************************************************************/

  template<typename UserType, typename RawType>
    requires(std::is_same_v<UserType, ChimeraTK::Void> || std::is_same_v<RawType, ChimeraTK::Void>)
  RawType Converter<UserType, RawType, SignificantBitsCase::generic, FractionalCase::integer, false>::toRaw(UserType) {
    return userTypeToNumeric<RawType>(ChimeraTK::Void());
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::RawConverter
