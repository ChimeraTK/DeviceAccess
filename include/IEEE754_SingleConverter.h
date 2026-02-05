// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericConverter.h"

#include <boost/numeric/conversion/cast.hpp>

#include <memory.h>

#include <cfloat> // for float limits

namespace ChimeraTK {

  /**
   * Needs to have the same interface as FixedPointConverter, except for the constructor. Converter for IEEE754 single
   * precision (32bit) floating point.
   */
  struct IEEE754_SingleConverter {
    template<typename CookedType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
    void vectorToCooked(
        const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, const COOKED_ITERATOR& cooked_begin) const {
      // Note: IEEE754_SingleConverter must be instantiable for all raw user types but can only be used for int32_t
      assert((std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int32_t>::value));

      static_assert(std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int8_t>::value ||
              std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int16_t>::value ||
              std::is_same<typename std::iterator_traits<RAW_ITERATOR>::value_type, int32_t>::value,
          "RAW_ITERATOR template argument must be an iterator with value type equal to int8_t, int16_t or int32_t.");

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
    CookedType scalarToCooked(int32_t const& raw) const {
      CookedType cooked;
      vectorToCooked<CookedType>(&raw, (&raw) + 1, &cooked);
      return cooked;
    }

    template<typename CookedType>
    uint32_t toRaw(CookedType cookedValue) const;

    explicit IEEE754_SingleConverter(const std::string& = "") {}

    // all IEEE754_SingleConverters are the same
    bool operator!=(const IEEE754_SingleConverter& /*other*/) const { return false; }
    bool operator==(const IEEE754_SingleConverter& /*other*/) const { return true; }
  };

  template<typename CookedType, typename RAW_ITERATOR, typename COOKED_ITERATOR>
  void IEEE754_SingleConverter::vectorToCooked_impl<CookedType, RAW_ITERATOR, COOKED_ITERATOR>::impl(
      const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, COOKED_ITERATOR cooked_begin) {
    for(auto it = raw_begin; it != raw_end; ++it) {
      // Step 1: convert the raw data to the "generic" representation in the CPU: float
      float genericRepresentation;
      memcpy(&genericRepresentation, &(*it), sizeof(float));

      // Step 2: convert the float to the cooked type
      *cooked_begin = numeric::convert<CookedType>(genericRepresentation);
      ++cooked_begin;
    }
  }

  template<typename CookedType>
  uint32_t IEEE754_SingleConverter::toRaw(CookedType cookedValue) const {
    // step 1: convert from cooked to the generic representation in the CPU
    // (float)
    float genericRepresentation = numeric::convert<float>(cookedValue);

    // step 2: reinterpret float to int32 to send it to the device
    int32_t rawValue;
    memcpy(&rawValue, &genericRepresentation, sizeof(float));

    return rawValue;
  }

  template<typename RAW_ITERATOR, typename COOKED_ITERATOR>
  struct IEEE754_SingleConverter::vectorToCooked_impl<std::string, RAW_ITERATOR, COOKED_ITERATOR> {
    static void impl(const RAW_ITERATOR& raw_begin, const RAW_ITERATOR& raw_end, COOKED_ITERATOR cooked_begin) {
      for(auto it = raw_begin; it != raw_end; ++it) {
        // Step 1: convert the raw data to the "generic" representation in the CPU: float
        float genericRepresentation;
        memcpy(&genericRepresentation, &(*it), sizeof(float));

        // Step 2: convert the float to the cooked type
        *cooked_begin = std::to_string(genericRepresentation);
        ++cooked_begin;
      }
    }
  };

  template<>
  [[nodiscard]] uint32_t IEEE754_SingleConverter::toRaw(std::string cookedValue) const;

} // namespace ChimeraTK
