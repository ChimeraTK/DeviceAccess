// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Exception.h"
#include "SupportedUserTypes.h"

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace ChimeraTK {

  /** The fixed point converter provides conversion functions
   *  between a user type and up to 32 bit fixed point (signed or unsigned).
   */

  template<typename RawType>
  class FixedPointConverter {
   public:
    /** The constructor defines the conversion factor.
     *  @param variableName The name of the variable. It is used in case an
     exception is thrown which significantly simplifies the debugging.
     *  @param nBits The number of total bits must not exceed 32.
     *  @param fractionalBits The number of fractional bits can range from
     *    -1024+nBits to 1021-nBits (in case of a double user type). For integer
     *    user types, no fractional bits are allowed.
     *  @param isSignedFlag Flag whether the most significant bit is interpreted
     as
     *    sign bit. Negative numbers are interpreted as two's complement
     *    number of the respective number of bits
     *    (i.e. in signed 6 bit, 0 fractional bits 0x3F is -1)
     */
    explicit FixedPointConverter(
        std::string variableName, unsigned int nBits = 32, int fractionalBits = 0, bool isSignedFlag = true);

    /** Conversion function from type T to fixed point.
     *  This conversion usually will introduce rounding errors due to this limited
     *  resolution of the fixed point number compared to the double.
     *  In case of a high number of fractional bits this can mean that the most
     *  significant digits are cut and only the 'noise' in the less significant
     * bits is represented.
     */
    template<typename UserType>
    RawType toRaw(UserType cookedValue) const;

    /**
     *  Conversion function from fixed-point values to type T.
     *  The two vectors passed must be of equal size (i.e. cookedValues must already be properly allocated).
     */
    template<typename UserType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
    void vectorToCooked(
        const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, const COOKED_ITERATOR& cooked_begin) const {
      static_assert(std::is_same<typename std::iterator_traits<RAW_ITERATOR>::iterator_category,
                        std::random_access_iterator_tag>::value,
          "RAW_ITERATOR template argument must be a random access iterator.");
      static_assert(std::is_same<typename std::iterator_traits<COOKED_ITERATOR>::iterator_category,
                        std::random_access_iterator_tag>::value,
          "COOKED_ITERATOR template argument must be a random access iterator.");
      static_assert(std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int8_t>::value ||
              std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int16_t>::value ||
              std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int32_t>::value,
          "RAW_ITERATOR template argument must be an iterator with value type equal to int8_t, int16_t or int32_t");
      static_assert(std::is_same<typename std::iterator_traits<COOKED_ITERATOR>::value_type, UserType>::value,
          "COOKED_ITERATOR template argument must be an iterator with value type equal to the UserType template "
          "argument.");
      vectorToCooked_impl<UserType, RAW_ITERATOR, COOKED_ITERATOR>::impl(*this, raw_begin, raw_end, cooked_begin);
    }
    template<typename UserType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
    struct vectorToCooked_impl {
      static void impl(const FixedPointConverter& fpc, const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end,
          COOKED_ITERATOR cooked_begin);
    };

    /** Inefficient convenience function for converting a single value to cooked */
    template<typename UserType>
    UserType scalarToCooked(int32_t const& raw) const {
      UserType cooked;
      vectorToCooked<UserType>(&raw, (&raw) + 1, &cooked);
      return cooked;
    }

    /** Read back the number of bits the converter is using. */
    [[nodiscard]] unsigned int getNBits() const { return _nBits; }

    /** Read back the fractional bits the converter is using. */
    [[nodiscard]] int getFractionalBits() const { return _fractionalBits; }

    /** Read back wether the conversion is using signed values. */
    [[nodiscard]] bool isSigned() const { return _isSigned; }

    /** Reconfigure the fixed point converter with new type information */
    void reconfigure(unsigned int nBits = 32, int fractionalBits = 0, bool isSignedFlag = true);

    /** Compare two fixed point converters. The variable name is ignored in this
     * comparison. */
    bool operator==(const FixedPointConverter& other) const {
      return _nBits == other._nBits && _fractionalBits == other._fractionalBits && _isSigned == other._isSigned;
    }
    bool operator!=(const FixedPointConverter& other) const { return !operator==(other); }

   private:
    std::string _variableName;
    unsigned int _nBits;
    int _fractionalBits;
    bool _isSigned;

    /// Coefficient containing the multiplication factor described by the
    /// fractional bits 2^fractionalBits
    double _fractionalBitsCoefficient;

    /// Coefficient containing the inverse multiplication factor described by the
    /// fractional bits 2^(-fractionalBits). Used to always multiply because this
    /// is faster than division in the floating point unit.
    double _inverseFractionalBitsCoefficient;

    RawType _signBitMask{};        ///< The bit which represents the sign
    RawType _usedBitsMask{};       ///< The bits which are used
    RawType _unusedBitsMask{};     ///< The bits which are not used
    RawType _bitShiftMask{};       ///< Mask with N most significant bits set, where N is
                                   ///< the number of factional bits
    RawType _bitShiftMaskSigned{}; ///< Mask with N most significant bits set, where N
                                   ///< is the number of factional bits + 1 if signed

    RawType _maxRawValue{}; ///< The maximum possible fixed point value
    RawType _minRawValue{}; ///< The minimum possible fixed point value

    /// maximum cooked values (depending on user type)
    userTypeMap _maxCookedValues;

    /// minimum cooked values (depending on user type)
    userTypeMap _minCookedValues;

    /// conversion branch for toCooked(). This allows to use a fast case statement instead of a complicated if in the
    /// time critical section
    FixedUserTypeMap<int> conversionBranch_toCooked;

    /// helper constant to avoid the "comparison always false" warning. will be
    /// always 0
    const static int zero;

    /// helper class to initialise coefficients etc. for all possible UserTypes
    class initCoefficients {
     public:
      explicit initCoefficients(FixedPointConverter* fpc) : _fpc(fpc) {}

      template<typename Pair>
      void operator()(Pair) const {
        // obtain UserType from given fusion::pair type
        using UserType = typename Pair::first_type;

        // compute conversion branches. Needs to be done before the subsequent calls to toCooked()!
        if(_fpc->_nBits == 16 && _fpc->_fractionalBits == 0 && !_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 9;
        }
        else if(_fpc->_nBits == 16 && _fpc->_fractionalBits == 0 && _fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 10;
        }
        else if(std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && !_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 1;
        }
        else if(std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && _fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 2;
        }
        else if(_fpc->_nBits == 16 && _fpc->_fractionalBits < 0 && _fpc->_fractionalBits > -16 && !_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 7;
        }
        else if(_fpc->_nBits == 16 && _fpc->_fractionalBits < 0 && _fpc->_fractionalBits > -16 && _fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 8;
        }
        else if(_fpc->_nBits == 16 && !_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 5;
        }
        else if(_fpc->_nBits == 16 && _fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 6;
        }
        else if(!_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 3;
        }
        else if(_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 4;
        }

        // compute minimum and maximum values in cooked representation
        try {
          boost::fusion::at_key<UserType>(_fpc->_minCookedValues) = _fpc->scalarToCooked<UserType>(_fpc->_minRawValue);
        }
        catch(boost::numeric::negative_overflow& e) {
          boost::fusion::at_key<UserType>(_fpc->_minCookedValues) = std::numeric_limits<UserType>::min();
        }
        try {
          boost::fusion::at_key<UserType>(_fpc->_maxCookedValues) = _fpc->scalarToCooked<UserType>(_fpc->_maxRawValue);
        }
        catch(boost::numeric::positive_overflow& e) {
          boost::fusion::at_key<UserType>(_fpc->_maxCookedValues) = std::numeric_limits<UserType>::max();
        }
      }

     private:
      FixedPointConverter* _fpc;
    };

    /** define round type for the boost::numeric::converter */
    template<class S>
    struct Round {
      static S nearbyint(S s) { return round(s); }

      using round_style = boost::mpl::integral_c<std::float_round_style, std::round_to_nearest>;
    };

    /** helper function to test if UserTyped value is negative without triggering a
     * compiler warning for unsigned user types */
    template<typename UserType, typename std::enable_if<std::is_signed<UserType>{}, int>::type = 0>
    bool isNegativeUserType(UserType value) const;
    template<typename UserType, typename std::enable_if<!std::is_signed<UserType>{}, int>::type = 0>
    bool isNegativeUserType(UserType value) const;

    // helper function: force unused leading bits to 0 for positive or 1 for negative numbers
    // NOLINTBEGIN(hicpp-signed-bitwise)
    // NOLINTBEGIN(bugprone-narrowing-conversions)
    // Turn off the linter warning. Yes, we are fiddling with the bit interpretation here, that's the whole point.
    void padUnusedBits(int32_t& rawValue) const {
      if(!(rawValue & _signBitMask)) {
        rawValue &= _usedBitsMask;
      }
      else {
        rawValue |= _unusedBitsMask;
      }
    }
    // NOLINTEND(bugprone-narrowing-conversions)
    // NOLINTEND(hicpp-signed-bitwise)
  };

  /********************************************************************************************************************/

  // FIXME: replace reinterpret_cast with bit_cast once C++20 is available for us.
  // Until then, turn off the linter warning about reinterpret_cast
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  template<typename RawType>
  template<typename UserType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
  void FixedPointConverter<RawType>::vectorToCooked_impl<UserType, RAW_ITERATOR, COOKED_ITERATOR>::impl(
      const FixedPointConverter& fpc, const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end,
      COOKED_ITERATOR cooked_begin) {
    // Handle integer and floating-point types differently.
    switch(boost::fusion::at_key<UserType>(fpc.conversionBranch_toCooked.table)) {
      case 1: { // std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && !_fpc->_isSigned
        std::transform(raw_begin, raw_end, cooked_begin, [&fpc](int32_t rawValue) {
          fpc.padUnusedBits(rawValue);
          return numericToUserType<UserType>(*(reinterpret_cast<uint32_t*>(&rawValue)));
        });
        break;
      }
      case 2: { // std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && _fpc->_isSigned
        std::transform(raw_begin, raw_end, cooked_begin, [&fpc](int32_t rawValue) {
          fpc.padUnusedBits(rawValue);
          return numericToUserType<UserType>(rawValue);
        });
        break;
      }
      case 9: { // _fpc->_nBits == 16 && _fpc->_fractionalBits == 0 && !_fpc->_isSigned
        std::transform(raw_begin, raw_end, cooked_begin, [](const auto& rawValue) {
          return numericToUserType<UserType>(*(reinterpret_cast<const uint16_t*>(&rawValue)));
        });
        break;
      }
      case 10: { //  _fpc->_nBits == 16 && _fpc->_fractionalBits == 0 && _fpc->_isSigned
        std::transform(raw_begin, raw_end, cooked_begin, [](const auto& rawValue) {
          return numericToUserType<UserType>(*(reinterpret_cast<const int16_t*>(&rawValue)));
        });
        break;
      }
      case 7: { // _fpc->_nBits == 16 && _fpc->_fractionalBits < 0  && _fpc->_fractionalBits > -16 && !_fpc->_isSigned
        const auto f = static_cast<uint32_t>(fpc._fractionalBitsCoefficient);
        std::transform(raw_begin, raw_end, cooked_begin, [f](const auto& rawValue) {
          return numericToUserType<UserType>(f * *(reinterpret_cast<const uint16_t*>(&rawValue)));
        });
        break;
      }
      case 8: { //  _fpc->_nBits == 16 && _fpc->_fractionalBits < 0 && _fpc->_fractionalBits > -16 && _fpc->_isSigned
        const auto f = static_cast<int32_t>(fpc._fractionalBitsCoefficient);
        std::transform(raw_begin, raw_end, cooked_begin, [f](const auto& rawValue) {
          return numericToUserType<UserType>(f * *(reinterpret_cast<const int16_t*>(&rawValue)));
        });
        break;
      }
      case 5: { // _fpc->_nBits == 16 && !_fpc->_isSigned
        const auto f = fpc._fractionalBitsCoefficient;
        std::transform(raw_begin, raw_end, cooked_begin, [f](const auto& rawValue) {
          return numericToUserType<UserType>(f * *(reinterpret_cast<const uint16_t*>(&rawValue)));
        });
        break;
      }
      case 6: { //  _fpc->_nBits == 16 && _fpc->_isSigned
        const auto f = fpc._fractionalBitsCoefficient;
        std::transform(raw_begin, raw_end, cooked_begin, [f](const auto& rawValue) {
          return numericToUserType<UserType>(f * *(reinterpret_cast<const int16_t*>(&rawValue)));
        });
        break;
      }
      case 3: { // !_fpc->_isSigned
        const auto f = fpc._fractionalBitsCoefficient;
        std::transform(raw_begin, raw_end, cooked_begin, [&fpc, f](int32_t rawValue) {
          fpc.padUnusedBits(rawValue);
          return numericToUserType<UserType>(f * *(reinterpret_cast<uint32_t*>(&rawValue)));
        });
        break;
      }
      case 4: { // _fpc->_isSigned
        const auto f = fpc._fractionalBitsCoefficient;
        std::transform(raw_begin, raw_end, cooked_begin, [&fpc, f](int32_t rawValue) {
          fpc.padUnusedBits(rawValue);
          auto ttt = numericToUserType<UserType>(f * rawValue);
          return ttt;
        });
        break;
      }
      default: {
        std::cerr << "Fixed point converter configuration is corrupt." << std::endl;
        std::terminate();
      }
    }
  }
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

  /********************************************************************************************************************/
  template<typename RawType>
  template<typename UserType>
  RawType FixedPointConverter<RawType>::toRaw(UserType cookedValue) const {
    if constexpr(std::is_same<UserType, std::string>::value) {
      if(_fractionalBits == 0) { // use integer conversion
        if(_isSigned) {
          return toRaw(std::stoi(cookedValue));
        }
        return toRaw(static_cast<uint32_t>(std::stoul(cookedValue))); // on some compilers, long might be a
                                                                      // different type than int...
      }
      return toRaw(std::stod(cookedValue));
    }
    else if constexpr(std::is_same<UserType, Boolean>::value) {
      if((bool)cookedValue) { // use integer conversion
        return 1.0;
      }
      return 0.0;
    }
    else if constexpr(std::is_same<UserType, Void>::value) {
      return 0.0;
    }

    else {
      // Do a range check first. The later overflow check in the conversion is not
      // sufficient, since we can have non-standard word sizes like 12 bits.
      if(cookedValue < boost::fusion::at_key<UserType>(_minCookedValues)) {
        return _minRawValue;
      }
      if(cookedValue > boost::fusion::at_key<UserType>(_maxCookedValues)) {
        return _maxRawValue;
      }

      // handle integer and floating-point types differently
      if(std::numeric_limits<UserType>::is_integer && _fractionalBits == 0) {
        // extract the sign and leave the positive number
        bool isNegative = isNegativeUserType(cookedValue);
        if(isNegative && !_isSigned) return _minRawValue;
        if(isNegative) {
          cookedValue = -(cookedValue + 1); // bit inversion, ~ operator cannot be used as UserType might be double
        }
        // cast into raw type
        auto rawValue = static_cast<RawType>(cookedValue);

        // handle sign
        if(_isSigned && isNegative) {
          rawValue = ~rawValue;
        }

        // return with bit mask applied
        return rawValue & static_cast<RawType>(_usedBitsMask);
      }
      // convert into double and scale by fractional bit coefficient
      double d_cooked = _inverseFractionalBitsCoefficient * static_cast<double>(cookedValue);

      // Convert into either signed or unsigned int32_t, depending on _isSigned,
      // so the conversion handles the sign correctly. Store always in a uint32_t,
      // since this is our raw type. The conversion will properly round, when
      // needed. Negative overflow exceptions need to be caught for some corner
      // cases (e.g. number of fractional bits >= number of bits in total).
      // Positive overflow cannot happen due to the range check above (the negative
      // branch has one more possible value).
      RawType raw;
      try {
        if(_isSigned) {
          using converter_signed = boost::numeric::converter<int32_t, double,
              boost::numeric::conversion_traits<int32_t, double>, boost::numeric::def_overflow_handler, Round<double>>;
          raw = converter_signed::convert(d_cooked);
        }
        else {
          using converter_unsigned = boost::numeric::converter<uint32_t, double,
              boost::numeric::conversion_traits<uint32_t, double>, boost::numeric::def_overflow_handler, Round<double>>;
          raw = static_cast<RawType>(converter_unsigned::convert(d_cooked));
        }
      }
      catch(boost::numeric::negative_overflow& e) {
        raw = _minRawValue;
      }

      // apply bit mask
      // NOLINTNEXTLINE(hicpp-signed-bitwise)
      return raw & _usedBitsMask;
    }
  }

  /********************************************************************************************************************/
  template<typename RawType>
  template<typename UserType, typename std::enable_if<std::is_signed<UserType>{}, int>::type>
  bool FixedPointConverter<RawType>::isNegativeUserType(UserType value) const {
    return static_cast<bool>(value < 0);
  }

  template<typename RawType>
  template<typename UserType, typename std::enable_if<!std::is_signed<UserType>{}, int>::type>
  bool FixedPointConverter<RawType>::isNegativeUserType(UserType /*value*/) const {
    return false;
  }

  /********************************************************************************************************************/
  template<typename RawType>
  template<typename RAW_ITERATOR, typename COOKED_ITERATOR>
  struct FixedPointConverter<RawType>::vectorToCooked_impl<std::string, RAW_ITERATOR, COOKED_ITERATOR> {
    static void impl(const FixedPointConverter& fpc, const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end,
        COOKED_ITERATOR cooked_begin) {
      if(fpc._fractionalBits == 0) { // use integer conversion
        if(fpc._isSigned) {
          std::vector<int32_t> intValues(raw_end - raw_begin);
          fpc.vectorToCooked<int32_t>(raw_begin, raw_end, intValues.begin());
          for(auto it : intValues) {
            *cooked_begin = std::to_string(it);
            ++cooked_begin;
          }
        }
        else {
          std::vector<uint32_t> uintValues(raw_end - raw_begin);
          fpc.vectorToCooked<uint32_t>(raw_begin, raw_end, uintValues.begin());
          for(auto it : uintValues) {
            *cooked_begin = std::to_string(it);
            ++cooked_begin;
          }
        }
      }
      else {
        std::vector<double> doubleValues(raw_end - raw_begin);
        fpc.vectorToCooked<double>(raw_begin, raw_end, doubleValues.begin());
        for(auto it : doubleValues) {
          *cooked_begin = std::to_string(it);
          ++cooked_begin;
        }
      }
    }
  };

  /********************************************************************************************************************/
  /* As the class is templated now, move implementation from the source file*/
  /********************************************************************************************************************/
  template<typename RawType>
  const int FixedPointConverter<RawType>::zero = 0;

  template<typename RawType>
  FixedPointConverter<RawType>::FixedPointConverter(
      std::string variableName, unsigned int nBits, int fractionalBits, bool isSignedFlag)
  : _variableName(std::move(variableName)), _nBits(nBits), _fractionalBits(fractionalBits), _isSigned(isSignedFlag),
    _fractionalBitsCoefficient(pow(2., -fractionalBits)), _inverseFractionalBitsCoefficient(pow(2., fractionalBits)) {
    reconfigure(nBits, fractionalBits, isSignedFlag);
  }

  template<typename RawType>
  void FixedPointConverter<RawType>::reconfigure(unsigned int nBits, int fractionalBits, bool isSignedFlag) {
    _nBits = nBits;
    _fractionalBits = fractionalBits;
    _isSigned = isSignedFlag;
    _fractionalBitsCoefficient = pow(2., -fractionalBits);
    _inverseFractionalBitsCoefficient = pow(2., fractionalBits);

    _bitShiftMaskSigned = 0; // Fixme unused variable.
    // some sanity checks
    if(nBits > 32) {
      std::stringstream errorMessage;
      errorMessage << "The number of bits must be <= 32, but is " << nBits;
      throw ChimeraTK::logic_error(errorMessage.str());
    }

    // For floating-point types: check if number of fractional bits are complying
    // with the dynamic range Note: positive fractional bits give us smaller
    // numbers and thus correspond to negative exponents!
    if((fractionalBits > -std::numeric_limits<double>::min_exponent - static_cast<int>(nBits)) ||
        (fractionalBits < -std::numeric_limits<double>::max_exponent + static_cast<int>(nBits))) {
      std::stringstream errorMessage;
      errorMessage << "The number of fractional bits exceeds the dynamic"
                   << " range of a double.";
      throw ChimeraTK::logic_error(errorMessage.str());
    }

    // compute mask for the signed bit
    // keep the mask at 0 if unsigned to simplify further calculations
    if(nBits > 0) {
      // NOLINTNEXTLINE(hicpp-signed-bitwise)
      _signBitMask = (_isSigned ? 1 << (nBits - 1) : 0x0); // the highest valid bit is the sign
    }
    else {
      _signBitMask = 0;
    }

    // compute masks of used and unused bits
    // by using 1L (64 bit) this also works for 32 bits, which needs 33 bits during the calculation
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    _usedBitsMask = static_cast<RawType>((1L << nBits) - 1L);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    _unusedBitsMask = ~_usedBitsMask;

    // compute bit shift mask, used to test if bit shifting for fractional bits
    // leads to an overflow
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    _bitShiftMask = static_cast<RawType>(~(0xFFFFFFFF >> abs(_fractionalBits)));

    // compute minimum and maximum value in raw representation
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    _maxRawValue = _usedBitsMask ^ _signBitMask; // bitwise xor: first bit is 0 if signed
                                                 // NOLINTNEXTLINE(hicpp-signed-bitwise)
    _minRawValue = _signBitMask;                 // if only the sign bit is on, it is the smallest
                                                 // possible value
                                                 // (0 if unsigned)

    // fill all user type depending values: min and max cooked values and
    // fractional bit coefficients note: we loop over one of the maps only, but
    // initCoefficients() will fill all maps!
    boost::fusion::for_each(_minCookedValues, initCoefficients(this));
  }

  using DEPRECATED_FIXEDPOINT_DEFAULT = int64_t;

  /********************************************************************************************************************/
} // namespace ChimeraTK
