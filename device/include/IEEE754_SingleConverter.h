#ifndef CHIMERATK_IEEE754_SINGLE_CONVERTER_H
#define CHIMERATK_IEEE754_SINGLE_CONVERTER_H

#include <float.h> // for float limits
#include <boost/numeric/conversion/cast.hpp>

#include "Exception.h"
#include "SupportedUserTypes.h"

namespace ChimeraTK {

  template<typename SourceType, typename DestType>
  struct RoundingRangeCheckingDataConverter {
    /** define round type for the boost::numeric::converter */
    template<class S>
    struct Round {
      static S nearbyint(S s) { return round(s); }

      typedef boost::mpl::integral_c<std::float_round_style, std::round_to_nearest> round_style;
    };

    typedef boost::numeric::converter<DestType, SourceType, boost::numeric::conversion_traits<DestType, SourceType>,
        boost::numeric::def_overflow_handler, Round<SourceType>>
        converter;
  };

  /** Needs to have the same interface as FixedPointConverter, except for the
   * constructor. Converter for IEEE754 single precision (32bit) floating point.
   */
  struct IEEE754_SingleConverter {
    template<typename CookedType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
    void vectorToCooked(
        const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, const COOKED_ITERATOR& cooked_begin) const {
      static_assert(std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int32_t>::value,
          "RAW_ITERATOR template argument must be an iterator with value type equal to int32_t.");
      static_assert(std::is_same<typename std::iterator_traits<COOKED_ITERATOR>::value_type, CookedType>::value,
          "COOKED_ITERATOR template argument must be an iterator with value type equal to the CookedType template "
          "argument.");
      vectorToCooked_impl<CookedType, RAW_ITERATOR, COOKED_ITERATOR>::impl(raw_begin, raw_end, cooked_begin);
    }

    template<typename CookedType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
    struct vectorToCooked_impl {
      static void impl(const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, COOKED_ITERATOR cooked_begin);
    };

    /** Inefficient convenience function for converting a single value to cooked */
    template<typename CookedType>
    CookedType scalarToCooked(int32_t raw) const {
      CookedType cooked;
      vectorToCooked<CookedType>(
          raw_iterator<int32_t>(&raw), raw_iterator<int32_t>((&raw) + 1), raw_iterator<CookedType>(&cooked));
      return cooked;
    }

    template<typename CookedType>
    uint32_t toRaw(CookedType cookedValue) const;

    IEEE754_SingleConverter(std::string = "") {}

    // all IEEE754_SingleConverters are the same
    bool operator!=(const IEEE754_SingleConverter& /*other*/) const { return false; }
  };

  template<typename CookedType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
  void IEEE754_SingleConverter::vectorToCooked_impl<CookedType, RAW_ITERATOR, COOKED_ITERATOR>::impl(
      const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, COOKED_ITERATOR cooked_begin) {
    for(auto it = raw_begin; it != raw_end; ++it) {
      // Step 1: convert the raw data to the "generic" representation in the CPU: float
      float genericRepresentation = *(reinterpret_cast<const float*>(&(*it)));

      // Step 2: convert the float to the cooked type
      *cooked_begin = RoundingRangeCheckingDataConverter<float, CookedType>::converter::convert(genericRepresentation);
      ++cooked_begin;
    }
  }

  template<typename CookedType>
  uint32_t IEEE754_SingleConverter::toRaw(CookedType cookedValue) const {
    // step 1: convert from cooked to the generic representation in the CPU
    // (float)
    float genericRepresentation;
    try {
      genericRepresentation = RoundingRangeCheckingDataConverter<CookedType, float>::converter::convert(cookedValue);
    }
    catch(boost::numeric::positive_overflow&) {
      genericRepresentation = FLT_MAX;
    }
    catch(boost::numeric::negative_overflow&) {
      genericRepresentation = -FLT_MAX;
    }

    // step 2: reinterpret float to int32 to send it to the device
    void* warningAvoider = &genericRepresentation;
    int32_t rawValue = *(reinterpret_cast<int32_t*>(warningAvoider));

    return rawValue;
  }

  template<typename RAW_ITERATOR, typename COOKED_ITERATOR>
  struct IEEE754_SingleConverter::vectorToCooked_impl<std::string, RAW_ITERATOR, COOKED_ITERATOR> {
    static void impl(const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, COOKED_ITERATOR cooked_begin) {
      for(auto it = raw_begin; it != raw_end; ++it) {
        // Step 1: convert the raw data to the "generic" representation in the CPU: float
        float genericRepresentation = *(reinterpret_cast<const float*>(&(*it)));

        // Step 2: convert the float to the cooked type
        *cooked_begin = std::to_string(genericRepresentation);
        ++cooked_begin;
      }
    }
  };

  template<>
  uint32_t IEEE754_SingleConverter::toRaw(std::string cookedValue) const;

} // namespace ChimeraTK

#endif // CHIMERATK_IEEE754_SINGLE_CONVERTER_H
