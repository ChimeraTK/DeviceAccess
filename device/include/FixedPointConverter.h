#ifndef CHIMERA_TK_FIXED_POINT_CONVERTER_H
#define CHIMERA_TK_FIXED_POINT_CONVERTER_H

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <type_traits>

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "Exception.h"
#include "SupportedUserTypes.h"

namespace ChimeraTK {

  /** The fixed point converter provides conversion functions
   *  between a user type and up to 32 bit fixed point (signed or unsigned).
   */
  class FixedPointConverter {
   public:
    /** The constructor defines the conversion factor.
     *  @param variableName The name of the variable. It is useds in case an
     exception is thrown which significantly simplifies the debugging.
     *  @param nBits The number of total bits must not exceed 32.
     *  @param fractionalBits The numer of fractional bits can range from
     *    -1024+nBits to 1021-nBits (in case of a double user type). For integer
     *    user types, no fractional bits are allowed.
     *  @param isSignedFlag Flag whether the most significant bit is interpreted
     as
     *    sign bit. Negative numbers are interpretes as two's complement
     *    number of the respective number of bits
     *    (i.e. in signed 6 bit, 0 fractional bits 0x3F is -1)
     */
    FixedPointConverter(
        std::string variableName, unsigned int nBits = 32, int fractionalBits = 0, bool isSignedFlag = true);

    /** Conversion function from fixed value to type T.
     *  In case the number of bits is less than 32, invalid leading bits are
     * ignored. Only the valid bits are interpreted.
     */
    template<typename UserType>
    UserType toCooked(uint32_t rawValue) const;

    /** Conversion function from type T to fixed point.
     *  This conversion usually will introduce rounding errors due to this limited
     *  resolution of the fixed point number compared to the double.
     *  In case of a high number of fractional bits this can mean that the most
     *  significant digits are cut and only the 'noise' in the less significant
     * bits is represented.
     */
    template<typename UserType>
    uint32_t toRaw(UserType cookedValue) const;

    /**
     *  Conversion function from fixed-point values to type T. This function is similar like toCooked() but processes an
     *  entire vector. The two vectors passed must be of equal size (i.e. cookedValues must already be properly
     *  allocated).
     */
    template<typename UserType>
    void vectorToCooked(const std::vector<uint32_t>& rawValues, std::vector<UserType>& cookedValues) const;

    /** Read back the number of bits the converter is using. */
    unsigned int getNBits() { return _nBits; }

    /** Read back the fractional bits the converter is using. */
    int getFractionalBits() { return _fractionalBits; }

    /** Read back wether the conversion is using signed values. */
    bool isSigned() { return _isSigned; }

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

    uint32_t _signBitMask;        ///< The bit which represents the sign
    uint32_t _usedBitsMask;       ///< The bits which are used
    uint32_t _unusedBitsMask;     ///< The bits which are not used
    uint32_t _bitShiftMask;       ///< Mask with N most significant bits set, where N is
                                  ///< the number of factional bits
    uint32_t _bitShiftMaskSigned; ///< Mask with N most significant bits set, where N
                                  ///< is the number of factional bits + 1 if signed

    uint32_t _maxRawValue; ///< The maximum possible fixed point value
    uint32_t _minRawValue; ///< The minimum possible fixed point value

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
      initCoefficients(FixedPointConverter* fpc) : _fpc(fpc) {}

      template<typename Pair>
      void operator()(Pair) const {
        // obtain UserType from given fusion::pair type
        typedef typename Pair::first_type UserType;

        // compute conversion branches. Needs to be done before the subsequent calls to toCooked()!
        if(std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && !_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 1;
        }
        else if(std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && _fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 2;
        }
        else if(!_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 3;
        }
        else if(_fpc->_isSigned) {
          boost::fusion::at_key<UserType>(_fpc->conversionBranch_toCooked.table) = 4;
        }

        // compute minimum and maximum values in cooked representation
        try {
          boost::fusion::at_key<UserType>(_fpc->_minCookedValues) =
              _fpc->template toCooked<UserType>(_fpc->_minRawValue);
        }
        catch(boost::numeric::negative_overflow& e) {
          boost::fusion::at_key<UserType>(_fpc->_minCookedValues) = std::numeric_limits<UserType>::min();
        }
        try {
          boost::fusion::at_key<UserType>(_fpc->_maxCookedValues) =
              _fpc->template toCooked<UserType>(_fpc->_maxRawValue);
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

      typedef boost::mpl::integral_c<std::float_round_style, std::round_to_nearest> round_style;
    };

    /** helper function to test if UserTyped value is negative without triggering a
     * compiler warning for unsigned user types */
    template<typename UserType, typename std::enable_if<std::is_signed<UserType>{}, int>::type = 0>
    bool isNegativeUserType(UserType value) const;
    template<typename UserType, typename std::enable_if<!std::is_signed<UserType>{}, int>::type = 0>
    bool isNegativeUserType(UserType value) const;

    // helper function: force unused leading bits to 0 for positive or 1 for negative numbers
    void padUnusedBits(uint32_t& rawValue) const {
      if(!(rawValue & _signBitMask)) {
        rawValue &= _usedBitsMask;
      }
      else {
        rawValue |= _unusedBitsMask;
      }
    }
  };

  /**********************************************************************************************************************/
  template<typename UserType>
  UserType FixedPointConverter::toCooked(uint32_t rawValue) const {
    UserType cooked;
    padUnusedBits(rawValue);

    // Handle integer and floating-point types differently.
    switch(boost::fusion::at_key<UserType>(conversionBranch_toCooked.table)) {
      case 1: { // std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && !_fpc->_isSigned
        cooked = boost::numeric_cast<UserType>(rawValue);
        break;
      }
      case 2: { // std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && _fpc->_isSigned
        cooked = boost::numeric_cast<UserType>(*(reinterpret_cast<int32_t*>(&rawValue)));
        break;
      }
      case 3: { // !_fpc->_isSigned
        // convert into double and scale to handle fractional bits
        double d_cooked = _fractionalBitsCoefficient * static_cast<double>(rawValue);

        // Convert into target type. This conversion takes care of proper rounding
        // when needed (and only then), and will throw
        // boost::numeric::positive_overflow resp. boost::numeric::negative_overflow
        // if the target range is exceeded.
        typedef boost::numeric::converter<UserType, double, boost::numeric::conversion_traits<UserType, double>,
            boost::numeric::def_overflow_handler, Round<double>>
            converter;
        cooked = converter::convert(d_cooked);
        break;
      }
      case 4: { // _fpc->_isSigned
        // convert into double and scale to handle fractional bits
        double d_cooked = _fractionalBitsCoefficient * static_cast<double>(*(reinterpret_cast<int32_t*>(&rawValue)));

        // Convert into target type. This conversion takes care of proper rounding
        // when needed (and only then), and will throw
        // boost::numeric::positive_overflow resp. boost::numeric::negative_overflow
        // if the target range is exceeded.
        typedef boost::numeric::converter<UserType, double, boost::numeric::conversion_traits<UserType, double>,
            boost::numeric::def_overflow_handler, Round<double>>
            converter;
        cooked = converter::convert(d_cooked);
        break;
      }
      default: {
        std::cerr << "Fixed point converter configuration is corrupt." << std::endl;
        std::terminate();
      }
    }
    return cooked;
  }

  /**********************************************************************************************************************/

  template<typename UserType>
  void FixedPointConverter::vectorToCooked(
      const std::vector<uint32_t>& rawValues, std::vector<UserType>& cookedValues) const {
    assert(rawValues.size() == cookedValues.size());

    // Handle integer and floating-point types differently.
    switch(boost::fusion::at_key<UserType>(conversionBranch_toCooked.table)) {
      case 1: { // std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && !_fpc->_isSigned
        for(size_t i = 0; i < rawValues.size(); ++i) {
          auto rawValue = rawValues[i];
          padUnusedBits(rawValue);
          cookedValues[i] = boost::numeric_cast<UserType>(rawValue);
        }
        break;
      }
      case 2: { // std::numeric_limits<UserType>::is_integer && _fpc->_fractionalBits == 0 && _fpc->_isSigned
        for(size_t i = 0; i < rawValues.size(); ++i) {
          auto rawValue = rawValues[i];
          padUnusedBits(rawValue);
          cookedValues[i] = boost::numeric_cast<UserType>(*(reinterpret_cast<int32_t*>(&rawValue)));
        }
        break;
      }
      case 3: { // !_fpc->_isSigned
        for(size_t i = 0; i < rawValues.size(); ++i) {
          auto rawValue = rawValues[i];
          padUnusedBits(rawValue);
          // convert into double and scale to handle fractional bits
          double d_cooked = _fractionalBitsCoefficient * static_cast<double>(rawValue);

          // Convert into target type. This conversion takes care of proper rounding
          // when needed (and only then), and will throw
          // boost::numeric::positive_overflow resp. boost::numeric::negative_overflow
          // if the target range is exceeded.
          typedef boost::numeric::converter<UserType, double, boost::numeric::conversion_traits<UserType, double>,
              boost::numeric::def_overflow_handler, Round<double>>
              converter;
          cookedValues[i] = converter::convert(d_cooked);
        }
        break;
      }
      case 4: { // _fpc->_isSigned
        for(size_t i = 0; i < rawValues.size(); ++i) {
          auto rawValue = rawValues[i];
          padUnusedBits(rawValue);
          // convert into double and scale to handle fractional bits
          double d_cooked = _fractionalBitsCoefficient * static_cast<double>(*(reinterpret_cast<int32_t*>(&rawValue)));

          // Convert into target type. This conversion takes care of proper rounding
          // when needed (and only then), and will throw
          // boost::numeric::positive_overflow resp. boost::numeric::negative_overflow
          // if the target range is exceeded.
          typedef boost::numeric::converter<UserType, double, boost::numeric::conversion_traits<UserType, double>,
              boost::numeric::def_overflow_handler, Round<double>>
              converter;
          cookedValues[i] = converter::convert(d_cooked);
        }
        break;
      }
      default: {
        std::cerr << "Fixed point converter configuration is corrupt." << std::endl;
        std::terminate();
      }
    }
  }

  /**********************************************************************************************************************/

  template<typename UserType>
  uint32_t FixedPointConverter::toRaw(UserType cookedValue) const {
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
      if(isNegative)
        cookedValue = -(cookedValue + 1); // bit inversion, ~ operator cannot be
                                          // used as UserType might be double

      // cast into raw type
      uint32_t rawValue = static_cast<uint32_t>(cookedValue);

      // handle sign
      if(_isSigned && isNegative) {
        rawValue = ~rawValue;
      }

      // return with bit mask applied
      return rawValue & _usedBitsMask;
    }
    else {
      // convert into double and scale by fractional bit coefficient
      double d_cooked = _inverseFractionalBitsCoefficient * static_cast<double>(cookedValue);

      // Convert into either signed or uinsigned int32_t, depending on _isSigned,
      // so the conversion handls the sign correctly. Store always in a uint32_t,
      // since this is our raw type. The conversion will properly round, when
      // needed. Negative overflow exceptions need to be cought for some corner
      // cases (e.g. number of fractional bits >= number of bits in total).
      // Positive overflow cannot happen due to the rangecheck above (the negative
      // branch has one more possible value).
      uint32_t raw;
      try {
        if(_isSigned) {
          typedef boost::numeric::converter<int32_t, double, boost::numeric::conversion_traits<int32_t, double>,
              boost::numeric::def_overflow_handler, Round<double>>
              converter_signed;
          raw = converter_signed::convert(d_cooked);
        }
        else {
          typedef boost::numeric::converter<uint32_t, double, boost::numeric::conversion_traits<uint32_t, double>,
              boost::numeric::def_overflow_handler, Round<double>>
              converter_unsigned;
          raw = converter_unsigned::convert(d_cooked);
        }
      }
      catch(boost::numeric::negative_overflow& e) {
        raw = _minRawValue;
      }

      // apply bit mask
      return raw & _usedBitsMask;
    }
  }

  /**********************************************************************************************************************/

  template<typename UserType, typename std::enable_if<std::is_signed<UserType>{}, int>::type>
  bool FixedPointConverter::isNegativeUserType(UserType value) const {
    if(value < 0) return true;
    return false;
  }

  template<typename UserType, typename std::enable_if<!std::is_signed<UserType>{}, int>::type>
  bool FixedPointConverter::isNegativeUserType(UserType /*value*/) const {
    return false;
  }

  /**********************************************************************************************************************/
  template<>
  std::string FixedPointConverter::toCooked<std::string>(uint32_t rawValue) const;

  /**********************************************************************************************************************/
  template<>
  void FixedPointConverter::vectorToCooked<std::string>(
      const std::vector<uint32_t>& rawValues, std::vector<std::string>& cookedValues) const;

  /**********************************************************************************************************************/
  template<>
  uint32_t FixedPointConverter::toRaw<std::string>(std::string cookedValue) const;

} // namespace ChimeraTK

#endif // CHIMERA_TK_FIXED_POINT_CONVERTER_H
