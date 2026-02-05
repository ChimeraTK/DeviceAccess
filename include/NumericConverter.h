// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Boolean.h"
#include "Void.h"

#include <cmath>
#include <concepts>
#include <limits>
#include <type_traits>

namespace ChimeraTK::numeric {

  /********************************************************************************************************************/

  /**
   * Concept to require an integral data type incl. ChimeraTK::Boolean and ChimeraTK::Void in templates
   */
  template<typename T>
  concept Integral = std::integral<T> || std::is_same_v<T, ChimeraTK::Boolean>;

  /**
   * Concept to require a numeric data type in templates
   */
  template<typename T>
  concept Arithmetic = Integral<T> || std::floating_point<T>;

  /**
   * Concept to require a numeric data type in templates, or ChimeraTK::Void
   */
  template<typename T>
  concept ArithmeticOrVoid = Integral<T> || std::floating_point<T> || std::is_same_v<T, ChimeraTK::Void>;

  /********************************************************************************************************************/

  /**
   * Replacement for std::round() which also works constexpr and also casts to the target type in the same step.
   */
  namespace detail {
    template<Integral TO, std::floating_point FROM>
    constexpr TO roundAndCast(FROM x) {
      if constexpr(!isBoolean<TO>) {
        if(std::isnan(x)) {
          if constexpr(std::is_signed_v<TO>) {
            return std::numeric_limits<TO>::lowest();
          }
          return std::numeric_limits<TO>::max();
        }
        // NOLINTNEXTLINE(bugprone-incorrect-roundings)
        return (x >= FROM(0)) ? TO(x + FROM(0.5)) : TO(x - FROM(0.5));
      }

      // Note: NaN will yield false
      return x >= 0.5 || x <= -0.5;
    }

    /******************************************************************************************************************/

    // Helper to check wether T_LEFT has a greater maximum value than T_RIGHT, without generating warnings due to
    // unsigned/signed comparisons and precision loss in casts.
    template<Arithmetic T_LEFT, Arithmetic T_RIGHT>
    constexpr bool greaterMaximum() {
      if constexpr(std::is_same_v<T_LEFT, T_RIGHT> || isBoolean<T_LEFT>) {
        return false;
      }
      else if constexpr(isBoolean<T_RIGHT>) {
        static_assert(!isBoolean<T_LEFT>);
        return true;
      }
      else if constexpr(std::is_floating_point_v<T_LEFT>) {
        return std::is_same_v<T_LEFT, double> || std::is_integral_v<T_RIGHT>;
      }
      else if constexpr(std::is_floating_point_v<T_RIGHT>) {
        static_assert(std::is_integral_v<T_LEFT>);
        return false;
      }
      else {
        static_assert(std::is_integral_v<T_LEFT> && std::is_integral_v<T_RIGHT>);
        return std::numeric_limits<T_LEFT>::max() > std::numeric_limits<T_RIGHT>::max();
      }
    }

    /******************************************************************************************************************/

    // Helper to check wether T_LEFT has a lesser (more negative) minimum value than T_RIGHT, without generating
    // warnings due to precision loss in casts.
    // This function only works for signed data types (unsigned is handled differently)
    template<Arithmetic T_LEFT, Arithmetic T_RIGHT>
    constexpr bool lesserMinimum() {
      static_assert(std::is_signed_v<T_LEFT> && std::is_signed_v<T_RIGHT>);
      if constexpr(std::is_same_v<T_LEFT, T_RIGHT>) {
        return false;
      }
      else if constexpr(std::is_floating_point_v<T_LEFT>) {
        return std::is_same_v<T_LEFT, double> || std::is_integral_v<T_RIGHT>;
      }
      else if constexpr(std::is_floating_point_v<T_RIGHT>) {
        static_assert(std::is_integral_v<T_LEFT>);
        return false;
      }
      else {
        static_assert(std::is_integral_v<T_LEFT> && std::is_integral_v<T_RIGHT>);
        return std::numeric_limits<T_LEFT>::lowest() < std::numeric_limits<T_RIGHT>::lowest();
      }
    }
  } // namespace detail

  /********************************************************************************************************************/

  /**
   * Convert numeric data types with proper rounding and clamping to the target value range.
   *
   * The exact behaviour is defined as follows:
   *
   * - integer target types: both positive and negative overflows clamp to the closest value of the target type.
   * - unsigned integer targets: negative input values give always 0
   * - double-to-single floats: overflows clamp, Inf and NaN is kept as is
   * - float-to-integer: rounding to nearest integer
   * - bool targets: any non-zero value (after rounding to nearest integer) is true (also negative values)
   */
  template<ArithmeticOrVoid TO, ArithmeticOrVoid FROM>
  constexpr TO convert(FROM value) {
    if constexpr(std::is_same_v<FROM, TO> || (isBoolean<FROM> && isBoolean<TO>)) {
      // fastpath for equal types
      return value;
    }
    if constexpr(std::is_same_v<FROM, ChimeraTK::Void> || std::is_same_v<TO, ChimeraTK::Void>) {
      // fastpath for Void, also preventing compilation errors in the true numeric part below
      return {};
    }
    else {
      if constexpr(!std::is_floating_point_v<TO> || std::is_floating_point_v<FROM>) {
        // Conversion into floating point works (almost) always as expected, so we need special handling for integer
        // targets only

        if constexpr(detail::greaterMaximum<FROM, TO>()) {
          // clamp to maximum value
          if(value >= FROM(std::numeric_limits<TO>::max())) {
            if constexpr(std::is_floating_point_v<FROM> && std::is_floating_point_v<TO>) {
              if(std::isinf(value)) {
                return std::numeric_limits<TO>::infinity();
              }
            }
            return std::numeric_limits<TO>::max();
          }
        }
        if constexpr(std::is_signed_v<FROM> && !std::is_signed_v<TO> && !isBoolean<TO>) {
          // clamp to 0 for unsigned target types
          if(value < 0) {
            return TO(0);
          }
        }
        if constexpr(std::is_signed_v<FROM> && std::is_signed_v<TO>) {
          if constexpr(detail::lesserMinimum<FROM, TO>()) {
            // clamp to lowest for signed source and target
            if(value <= FROM(std::numeric_limits<TO>::lowest())) {
              if constexpr(std::is_floating_point_v<FROM> && std::is_floating_point_v<TO>) {
                if(std::isinf(value)) {
                  return -std::numeric_limits<TO>::infinity();
                }
              }
              return std::numeric_limits<TO>::lowest();
            }
          }
        }

        // Hint: no need for any clamping on minimum side for unsigned source!

        if constexpr(std::is_floating_point_v<FROM> && !std::is_floating_point_v<TO>) {
          // conversion from floating point into integer: need to round
          return detail::roundAndCast<TO>(value);
        }
      }

      return TO(value);
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::numeric
