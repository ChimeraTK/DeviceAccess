// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <cmath>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestNumericConverter
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <NumericConverter.h>

using namespace ChimeraTK::numeric;

/**********************************************************************************************************************/

// List of types to test with. ChimeraTK::Void is tested separately.
using IntTypes =
    std::tuple<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, bool, ChimeraTK::Boolean>;
using FloatTypes = std::tuple<float, double>;

/**********************************************************************************************************************/

// Helper function to iterate over the types of an std::tuple
template<typename Tuple, typename F>
void forEachType(F&& f) {
  []<std::size_t... Is>(F&& g, std::index_sequence<Is...>) {
    (g.template operator()<std::tuple_element_t<Is, Tuple>>(), ...);
  }(std::forward<F>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
};

/**********************************************************************************************************************/

// Note: This test does almost nothing at runtime, almost all checks are implemented as static_asserts at compile time.
BOOST_AUTO_TEST_CASE(TestNumericConverter) {
  // Conversion from int to float
  forEachType<IntTypes>([]<typename I>() {
    forEachType<FloatTypes>([]<typename F>() {
      // Note: we might not always distinguish "std::numeric_limits<I>::max()" from "std::numeric_limits<I>::max() - 1"
      // etc. in the floating point type, but the same imprecision must happen here on both sides of the ==.
      static_assert(convert<F>(std::numeric_limits<I>::max()) == F(std::numeric_limits<I>::max()));
      static_assert(convert<F>(std::numeric_limits<I>::lowest()) == F(std::numeric_limits<I>::lowest()));
      static_assert(convert<F>(std::numeric_limits<I>::max() - 1) == F(std::numeric_limits<I>::max() - 1));
      static_assert(convert<F>(std::numeric_limits<I>::lowest() + 1) == F(std::numeric_limits<I>::lowest() + 1));

      static_assert(convert<F>(I(1)) == F(1));
      static_assert(convert<F>(I(0)) == F(0));
      if constexpr(!ChimeraTK::isBoolean<I>) {
        static_assert(convert<F>(I(42)) == F(42));
      }

      if constexpr(std::is_signed_v<I>) {
        static_assert(convert<F>(I(-1)) == F(-1));
        static_assert(convert<F>(I(-120)) == F(-120));
      }
    });
  });

  // Conversion from float to int
  forEachType<IntTypes>([]<typename I>() {
    forEachType<FloatTypes>([]<typename F>() {
      if constexpr(!std::is_same_v<I, int64_t> && !std::is_same_v<I, uint64_t> &&
          (std::is_same_v<F, double> || (!std::is_same_v<I, int32_t> && !std::is_same_v<I, uint32_t>))) {
        // these checks work only if lowest() and max() of the I type can be exactly represented in the F type
        static_assert(convert<I>(F(std::numeric_limits<I>::max())) == std::numeric_limits<I>::max());
        static_assert(convert<I>(F(std::numeric_limits<I>::max() - 1)) == std::numeric_limits<I>::max() - 1);

        static_assert(convert<I>(F(std::numeric_limits<I>::lowest())) == std::numeric_limits<I>::lowest());
        static_assert(convert<I>(F(std::numeric_limits<I>::lowest() + 1)) == std::numeric_limits<I>::lowest() + 1);

        // check proper rounding for big values
        static_assert(convert<I>(F(std::numeric_limits<I>::max()) - F(0.51)) == std::numeric_limits<I>::max() - 1);
        static_assert(convert<I>(F(std::numeric_limits<I>::max()) - F(0.49)) == std::numeric_limits<I>::max());

        static_assert(
            convert<I>(F(std::numeric_limits<I>::lowest()) + F(0.51)) == std::numeric_limits<I>::lowest() + 1);
        static_assert(convert<I>(F(std::numeric_limits<I>::lowest()) + F(0.49)) == std::numeric_limits<I>::lowest());
      }
      // the next two checks might be insensitive for some types due to limited floating point precision
      static_assert(convert<I>(F(std::numeric_limits<I>::max()) + F(0.49)) == std::numeric_limits<I>::max());
      static_assert(convert<I>(F(std::numeric_limits<I>::max()) + F(0.51)) == std::numeric_limits<I>::max());
      // make sure the next one does become insensitive due to limited precision
      static_assert(F(std::numeric_limits<I>::max()) + F(1.E13) != F(std::numeric_limits<I>::max()));
      static_assert(convert<I>(F(std::numeric_limits<I>::max()) + F(1.E13)) == std::numeric_limits<I>::max());

      static_assert(convert<I>(F(std::numeric_limits<I>::lowest()) - F(0.49)) == std::numeric_limits<I>::lowest());
      if constexpr(!ChimeraTK::isBoolean<I>) {
        static_assert(convert<I>(F(std::numeric_limits<I>::lowest()) - F(0.51)) == std::numeric_limits<I>::lowest());
        static_assert(convert<I>(F(std::numeric_limits<I>::lowest()) - F(100000.)) == std::numeric_limits<I>::lowest());
      }
      else {
        static_assert(convert<I>(-F(0.51)) == true);
        static_assert(convert<I>(-F(100000.)) == true);
      }

      static_assert(convert<I>(F(1)) == I(1));
      static_assert(convert<I>(F(0)) == I(0));
      static_assert(convert<I>(F(42)) == I(42));
      if constexpr(std::is_signed_v<I>) {
        static_assert(convert<I>(F(-1)) == I(-1));
        static_assert(convert<I>(F(-120)) == I(-120));
      }
      else {
        if constexpr(!ChimeraTK::isBoolean<I>) {
          static_assert(convert<I>(F(-1)) == 0);
          static_assert(convert<I>(F(-120)) == 0);
        }
        else {
          static_assert(convert<I>(F(-1)) == true);
          static_assert(convert<I>(F(-120)) == true);
        }
      }

      // check proper rounding
      static_assert(convert<I>(F(0.49999)) == 0);
      static_assert(convert<I>(F(0.50001)) == 1);
      static_assert(convert<I>(F(1.49999)) == 1);
      if constexpr(!ChimeraTK::isBoolean<I>) {
        static_assert(convert<I>(F(1.50001)) == 2);
      }
      static_assert(convert<I>(F(-0.49999)) == 0);
      if constexpr(std::is_signed_v<I>) {
        static_assert(convert<I>(F(-0.50001)) == -1);
        static_assert(convert<I>(F(-1.49999)) == -1);
        static_assert(convert<I>(F(-1.50001)) == -2);
      }
      else {
        if constexpr(!ChimeraTK::isBoolean<I>) {
          static_assert(convert<I>(F(-0.50001)) == 0);
          static_assert(convert<I>(F(-1.49999)) == 0);
          static_assert(convert<I>(F(-1.50001)) == 0);
        }
        else {
          static_assert(convert<I>(F(-0.50001)) == true);
          static_assert(convert<I>(F(-1.49999)) == true);
          static_assert(convert<I>(F(-1.50001)) == true);
        }
      }

      // check Inf and NaN
      static_assert(convert<I>(std::numeric_limits<F>::infinity()) == std::numeric_limits<I>::max());
      if constexpr(!ChimeraTK::isBoolean<I>) {
        static_assert(convert<I>(-std::numeric_limits<F>::infinity()) == std::numeric_limits<I>::lowest());
      }
      else {
        static_assert(convert<I>(-std::numeric_limits<F>::infinity()) == true);
      }

      if constexpr(std::is_signed_v<I>) {
        static_assert(convert<I>(std::numeric_limits<F>::quiet_NaN()) == std::numeric_limits<I>::lowest());
      }
      else {
        if constexpr(!ChimeraTK::isBoolean<I>) {
          static_assert(convert<I>(std::numeric_limits<F>::quiet_NaN()) == std::numeric_limits<I>::max());
        }
        else {
          static_assert(convert<I>(std::numeric_limits<F>::quiet_NaN()) == false);
        }
      }
    });
  });

  // Conversion from int to int
  forEachType<IntTypes>([]<typename I1>() {
    forEachType<IntTypes>([]<typename I2>() {
      if constexpr(detail::greaterMaximum<I2, I1>()) {
        // I2 can represent bigger values than I1
        static_assert(convert<I2>(std::numeric_limits<I1>::max()) == std::numeric_limits<I1>::max());
        static_assert(convert<I2>(std::numeric_limits<I1>::max() - 1) == std::numeric_limits<I1>::max() - 1);

        static_assert(convert<I1>(I2(std::numeric_limits<I1>::max()) + 1) == std::numeric_limits<I1>::max());
        static_assert(convert<I1>(std::numeric_limits<I2>::max()) == std::numeric_limits<I1>::max());
      }

      static_assert(convert<I1>(I2(1)) == 1);
      static_assert(convert<I1>(I2(0)) == 0);

      if constexpr(std::is_signed_v<I1> && std::is_signed_v<I2>) {
        // both are signed
        if constexpr(std::numeric_limits<I2>::lowest() < std::numeric_limits<I1>::lowest()) {
          // I2 can represent more negative values than I1
          static_assert(convert<I2>(std::numeric_limits<I1>::lowest()) == std::numeric_limits<I1>::lowest());
          static_assert(convert<I2>(std::numeric_limits<I1>::lowest() + 1) == std::numeric_limits<I1>::lowest() + 1);

          static_assert(convert<I1>(I2(std::numeric_limits<I1>::lowest()) - 1) == std::numeric_limits<I1>::lowest());
          static_assert(convert<I1>(std::numeric_limits<I2>::lowest()) == std::numeric_limits<I1>::lowest());
        }
      }

      if constexpr(std::is_signed_v<I1> && !std::is_signed_v<I2>) {
        // only I1 is signed
        if constexpr(!ChimeraTK::isBoolean<I2>) {
          static_assert(convert<I2>(std::numeric_limits<I1>::lowest()) == 0);
          static_assert(convert<I2>(std::numeric_limits<I1>::lowest() + 1) == 0);
          static_assert(convert<I2>(-1) == 0);
        }
        else {
          // any non-zero value, including negative values are considered "true"
          static_assert(convert<I2>(std::numeric_limits<I1>::lowest()) == true);
          static_assert(convert<I2>(std::numeric_limits<I1>::lowest() + 1) == true);
          static_assert(convert<I2>(-1) == true);
        }
      }
    });
  });

  // conversion from float to float
  forEachType<FloatTypes>([]<typename F1>() {
    forEachType<FloatTypes>([]<typename F2>() {
      if constexpr(detail::greaterMaximum<F2, F1>()) {
        // F2 = double, F1 = float
        static_assert(convert<F1>(std::numeric_limits<F2>::max()) == std::numeric_limits<F1>::max());
        static_assert(convert<F1>(std::numeric_limits<F2>::lowest()) == std::numeric_limits<F1>::lowest());

        static_assert(convert<F2>(std::numeric_limits<F1>::max()) == F2(std::numeric_limits<F1>::max()));
        static_assert(convert<F2>(std::numeric_limits<F1>::lowest()) == F2(std::numeric_limits<F1>::lowest()));
      }

      static_assert(convert<F2>(F1(0.)) == F2(0.));
      static_assert(convert<F2>(F1(1.)) == F2(1.));
      static_assert(convert<F2>(F1(-1.)) == F2(-1.));
      static_assert(convert<F2>(F1(0.12345)) == F2(F1(0.12345)));

      // check retention of sign bit for zero
      // (Note: std::signbit isn't yet constexpr with clang on Ubuntu 24)
      BOOST_TEST(std::signbit(convert<F2>(F1(0.))) == 0);  // signbit 0 means "positive"
      constexpr F2 result = convert<F2>(F1(0.) / F1(-1.)); // "-0." will not give us a negative 0 in C++
      static_assert(result == 0);                          // negative and positive zero compare equal in C++
      BOOST_TEST(std::signbit(result) == 1);               // signbit 1 means "negative"

      static_assert(std::isnan(convert<F2>(std::numeric_limits<F1>::quiet_NaN())));
      static_assert(std::isinf(convert<F2>(std::numeric_limits<F1>::infinity())));
      static_assert(convert<F2>(std::numeric_limits<F1>::infinity()) == std::numeric_limits<F2>::infinity());
      static_assert(convert<F2>(-std::numeric_limits<F1>::infinity()) == -std::numeric_limits<F2>::infinity());
    });
  });

  // conversion from/to Void
  forEachType<FloatTypes>([]<typename F>() {
    static_assert(convert<F>(ChimeraTK::Void{}) == F(0));
    constexpr auto result1 = convert<ChimeraTK::Void>(F(0.0));
    static_assert(std::is_same_v<decltype(result1), const ChimeraTK::Void>);
    constexpr auto result2 = convert<ChimeraTK::Void>(F(123.456));
    static_assert(std::is_same_v<decltype(result2), const ChimeraTK::Void>);
  });
  forEachType<IntTypes>([]<typename I>() {
    static_assert(convert<I>(ChimeraTK::Void{}) == I(0));
    constexpr auto result1 = convert<ChimeraTK::Void>(I(0));
    static_assert(std::is_same_v<decltype(result1), const ChimeraTK::Void>);
    constexpr auto result2 = convert<ChimeraTK::Void>(I(123));
    static_assert(std::is_same_v<decltype(result2), const ChimeraTK::Void>);
  });
}

/**********************************************************************************************************************/
