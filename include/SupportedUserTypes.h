// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Wrapper Class to avoid vector<bool> problems
   */
  class Boolean {
   public:
    Boolean() : m_value() {}
    // We want implicit construction and conversion. Turn off the linter warnings.
    // NOLINTBEGIN(hicpp-explicit-conversions, google-explicit-constructor)
    Boolean(bool value) : m_value(value) {}

    operator const bool&() const { return m_value; }
    operator bool&() { return m_value; }
    // NOLINTEND(hicpp-explicit-conversions, google-explicit-constructor)
    // TODO: test user types to numeric etc

   private:
    bool m_value;
  };

  /********************************************************************************************************************/

  inline std::istream& operator>>(std::istream& is, Boolean& value) {
    std::string data;
    is >> data;

    std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) { return std::tolower(c); });

    if(data == "false" || data == "0" || data.empty()) {
      value = false;
    }
    else {
      value = true;
    }
    return is;
  }

  /********************************************************************************************************************/

  // Define ChimeraTK::to_string to convert Boolean into string. We cannot define std::to_string(Boolean), as it would
  // violate the C++ standard. The right definition can be autoselected with
  // "using std::to_string; using ChimeraTK::to_string;".
  inline std::string to_string(Boolean& value) {
    if(value) {
      return "true";
    }
    return "false";
  }

  /********************************************************************************************************************/

  /**
   * Wrapper Class for void. return is always  0.
   */
  class Void {
   public:
    Void() = default;
    Void& operator=(const Void&) = default;
    Void(const Void&) = default;
  };

  /********************************************************************************************************************/

  inline std::istream& operator>>(std::istream& is, __attribute__((unused)) Void& value) {
    return is;
  }

  /********************************************************************************************************************/

  inline std::ostream& operator<<(std::ostream& os, __attribute__((unused)) Void& value) {
    os << 0;
    return os;
  }

  /********************************************************************************************************************/
  /** Helper classes for the conversion functions below */
  namespace detail {
    template<typename UserType, typename NUMERIC>
    struct numericToUserType_impl {
      static UserType impl(NUMERIC value);
    };

    template<typename NUMERIC>
    struct numericToUserType_impl<std::string, NUMERIC> {
      static std::string impl(NUMERIC value);
    };

    template<typename NUMERIC>
    struct numericToUserType_impl<Void, NUMERIC> {
      static Void impl(NUMERIC value);
    };

    template<typename NUMERIC>
    struct numericToUserType_impl<double, NUMERIC> {
      static double impl(NUMERIC value);
    };

    template<typename NUMERIC>
    struct numericToUserType_impl<float, NUMERIC> {
      static float impl(NUMERIC value);
    };

    template<typename UserType, typename NUMERIC>
    struct userTypeToNumeric_impl {
      static NUMERIC impl(UserType value);
    };

    template<typename NUMERIC>
    struct userTypeToNumeric_impl<std::string, NUMERIC> {
      static NUMERIC impl(const std::string& value);
    };

    template<typename NUMERIC>
    struct userTypeToNumeric_impl<Void, NUMERIC> {
      static NUMERIC impl(const Void& value);
    };

    template<typename UserTypeReturn, typename UserTypeParameter>
    struct userTypeToUserType_impl {
      static UserTypeReturn impl(UserTypeParameter value);
    };

    template<>
    struct userTypeToUserType_impl<std::string, std::string> {
      static std::string impl(std::string value) { return value; }
    };

    template<typename UserTypeReturn>
    struct userTypeToUserType_impl<UserTypeReturn, std::string> {
      static UserTypeReturn impl(std::string value);
    };

    template<typename UserTypeReturn>
    struct userTypeToUserType_impl<UserTypeReturn, Void> {
      static UserTypeReturn impl(Void value);
    };

    template<typename UserTypeParameter>
    struct userTypeToUserType_impl<Void, UserTypeParameter> {
      static Void impl(UserTypeParameter value);
    };

    template<>
    struct userTypeToUserType_impl<Void, Void> {
      static Void impl(Void value) { return value; }
    };

    template<>
    struct userTypeToUserType_impl<std::string, Void> {
      static std::string impl(__attribute__((unused)) Void value) { return std::to_string(0); }
    };

    template<>
    struct userTypeToUserType_impl<Void, std::string> {
      static Void impl(__attribute__((unused)) const std::string& value) { return {}; }
    };

    template<class S>
    struct Round {
      static S nearbyint(S s) { return std::round(s); }
      using round_style = boost::mpl::integral_c<std::float_round_style, std::round_to_nearest>;
    };

    template<>
    struct Round<Boolean> {
      static Boolean nearbyint(Boolean s) { return s; }
      using round_style = boost::mpl::integral_c<std::float_round_style, std::round_to_nearest>;
    };

    template<>
    struct Round<Void> {
      static Void nearbyint(__attribute__((unused)) Void s) { return s; }
      using round_style = boost::mpl::integral_c<std::float_round_style, std::round_to_nearest>;
    };

  } // namespace detail

  /********************************************************************************************************************/

  /** Helper function to convert numeric data into any UserType (even if it is a string etc.). The conversion is done
   *  with proper rounding and range checking. It will throw boost::numeric::positive_overflow resp.
   *  boost::numeric::negative_overflow if the data is out of range. */
  template<typename UserType, typename NUMERIC>
  UserType numericToUserType(NUMERIC value) {
    return detail::numericToUserType_impl<UserType, NUMERIC>::impl(value);
  }

  /********************************************************************************************************************/

  /** Helper function to convert numeric data into any UserType (even if it is a string etc.). The conversion is done
   *  with proper rounding and range checking. It will throw boost::numeric::positive_overflow resp.
   *  boost::numeric::negative_overflow if the data is out of range. */
  template<typename NUMERIC, typename UserType>
  NUMERIC userTypeToNumeric(UserType value) {
    return detail::userTypeToNumeric_impl<UserType, NUMERIC>::impl(value);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename NUMERIC>
  UserType detail::numericToUserType_impl<UserType, NUMERIC>::impl(NUMERIC value) {
    typedef boost::numeric::converter<UserType, NUMERIC, boost::numeric::conversion_traits<UserType, NUMERIC>,
        boost::numeric::def_overflow_handler, Round<NUMERIC>>
        converter;
    // There seems to be no other way to alter the value on negative/positive overflow than using a try-catch-block
    // here. The overflow_handler is a stateless class and hence can either throw another exception or do nothing.
    // It does not have any influence on the converted value, and it cannot transport the information out differently.
    try {
      return converter::convert(value);
    }
    catch(boost::numeric::negative_overflow&) {
      return std::numeric_limits<UserType>::min();
    }
    catch(boost::numeric::positive_overflow&) {
      return std::numeric_limits<UserType>::max();
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename NUMERIC>
  NUMERIC detail::userTypeToNumeric_impl<UserType, NUMERIC>::impl(UserType value) {
    typedef boost::numeric::converter<NUMERIC, UserType, boost::numeric::conversion_traits<NUMERIC, UserType>,
        boost::numeric::def_overflow_handler, Round<UserType>>
        converter;
    // There seems to be no other way to alter the value on negative/positive overflow than using a try-catch-block
    // here. The overflow_handler is a stateless class and hence can either throw another exception or do nothing.
    // It does not have any influence on the converted value, and it cannot transport the information out differently.
    try {
      return converter::convert(value);
    }
    catch(boost::numeric::negative_overflow&) {
      return std::numeric_limits<NUMERIC>::min();
    }
    catch(boost::numeric::positive_overflow&) {
      return std::numeric_limits<NUMERIC>::max();
    }
  }

  /********************************************************************************************************************/

  template<typename NUMERIC>
  inline double detail::numericToUserType_impl<double, NUMERIC>::impl(NUMERIC value) {
    return static_cast<double>(value);
  }

  /********************************************************************************************************************/

  template<typename NUMERIC>
  inline float detail::numericToUserType_impl<float, NUMERIC>::impl(NUMERIC value) {
    return static_cast<float>(value);
  }

  /********************************************************************************************************************/

  template<typename NUMERIC>
  std::string detail::numericToUserType_impl<std::string, NUMERIC>::impl(NUMERIC value) {
    using std::to_string;
    return to_string(value);
  }

  /********************************************************************************************************************/
  template<typename NUMERIC>
  Void detail::numericToUserType_impl<Void, NUMERIC>::impl(__attribute__((unused)) NUMERIC value) {
    return {};
  }

  /********************************************************************************************************************/

  template<typename NUMERIC>
  NUMERIC detail::userTypeToNumeric_impl<std::string, NUMERIC>::impl(const std::string& value) {
    if constexpr(std::is_same<NUMERIC, Boolean>::value) {
      // special treatment for Boolean
      std::stringstream ss(value);
      Boolean converted;
      ss >> converted;
      return converted;
    }

    if constexpr(!std::is_same<NUMERIC, int8_t>::value && !std::is_same<NUMERIC, uint8_t>::value) {
      NUMERIC v;
      std::stringstream ss(value);
      ss >> v;
      return v;
    }
    else {
      // int8_t and uint8_t are interpreted as char resp. unsigned char, in which case the stream operator does the
      // wrong thing...
      int temp;
      try {
        temp = std::stoi(value);
      }
      catch(...) {
        // ignore any parsing errors and just return 0 in that case
        temp = 0;
      }
      return detail::userTypeToNumeric_impl<decltype(temp), NUMERIC>::impl(temp);
    }
  }

  /********************************************************************************************************************/

  template<typename NUMERIC>
  NUMERIC detail::userTypeToNumeric_impl<Void, NUMERIC>::impl(__attribute__((unused)) const Void& value) {
    auto temp = 0.0;
    return detail::userTypeToNumeric_impl<decltype(temp), NUMERIC>::impl(temp);
  }

  /********************************************************************************************************************/

  /** Helper function to convert any UserType data into any other UserType (even if it is a string etc.). The conversion
   *  is done with proper rounding and range checking for numeric types. It will return the closed possible value if
   *  the value is out of range. */
  template<typename UserTypeReturn, typename UserTypeParameter>
  UserTypeReturn userTypeToUserType(UserTypeParameter value) {
    return detail::userTypeToUserType_impl<UserTypeReturn, UserTypeParameter>::impl(value);
  }

  /********************************************************************************************************************/

  template<typename UserTypeReturn, typename UserTypeParameter>
  inline UserTypeReturn detail::userTypeToUserType_impl<UserTypeReturn, UserTypeParameter>::impl(
      UserTypeParameter value) {
    return numericToUserType<UserTypeReturn>(value);
  }

  /********************************************************************************************************************/

  template<typename UserTypeReturn>
  inline UserTypeReturn detail::userTypeToUserType_impl<UserTypeReturn, std::string>::impl(std::string value) {
    return userTypeToNumeric<UserTypeReturn>(value);
  }

  /********************************************************************************************************************/

  template<typename UserTypeReturn>
  inline UserTypeReturn detail::userTypeToUserType_impl<UserTypeReturn, Void>::impl(
      __attribute__((unused)) Void value) {
    return numericToUserType<UserTypeReturn>(0);
  }

  /********************************************************************************************************************/

  template<typename UserTypeParameter>
  inline Void detail::userTypeToUserType_impl<Void, UserTypeParameter>::impl(
      __attribute__((unused)) UserTypeParameter value) {
    return {};
  }

  /********************************************************************************************************************/

  /** Map of UserType to value of the UserType. Used e.g. by the FixedPointConverter to store coefficients etc. in
   *  dependence of the UserType. */
  using userTypeMap = boost::fusion::map<boost::fusion::pair<int8_t, int8_t>, boost::fusion::pair<uint8_t, uint8_t>,
      boost::fusion::pair<int16_t, int16_t>, boost::fusion::pair<uint16_t, uint16_t>,
      boost::fusion::pair<int32_t, int32_t>, boost::fusion::pair<uint32_t, uint32_t>,
      boost::fusion::pair<int64_t, int64_t>, boost::fusion::pair<uint64_t, uint64_t>, boost::fusion::pair<float, float>,
      boost::fusion::pair<double, double>, boost::fusion::pair<std::string, std::string>,
      boost::fusion::pair<Boolean, Boolean>, boost::fusion::pair<Void, Void>>;

  /** Just like userTypeMap, only without the ChimeraTK::Void type. */
  using userTypeMapNoVoid = boost::fusion::map<boost::fusion::pair<int8_t, int8_t>,
      boost::fusion::pair<uint8_t, uint8_t>, boost::fusion::pair<int16_t, int16_t>,
      boost::fusion::pair<uint16_t, uint16_t>, boost::fusion::pair<int32_t, int32_t>,
      boost::fusion::pair<uint32_t, uint32_t>, boost::fusion::pair<int64_t, int64_t>,
      boost::fusion::pair<uint64_t, uint64_t>, boost::fusion::pair<float, float>, boost::fusion::pair<double, double>,
      boost::fusion::pair<std::string, std::string>, boost::fusion::pair<Boolean, Boolean>>;

  /** Map of UserType to a value of a single type (same for evey user type) */
  template<typename TargetType>
  class FixedUserTypeMap {
   public:
    boost::fusion::map<boost::fusion::pair<int8_t, TargetType>, boost::fusion::pair<uint8_t, TargetType>,
        boost::fusion::pair<int16_t, TargetType>, boost::fusion::pair<uint16_t, TargetType>,
        boost::fusion::pair<int32_t, TargetType>, boost::fusion::pair<uint32_t, TargetType>,
        boost::fusion::pair<int64_t, TargetType>, boost::fusion::pair<uint64_t, TargetType>,
        boost::fusion::pair<float, TargetType>, boost::fusion::pair<double, TargetType>,
        boost::fusion::pair<std::string, TargetType>, boost::fusion::pair<Boolean, TargetType>,
        boost::fusion::pair<Void, TargetType>>
        table;
  };

  /** Just like TemplateUserTypeMap, only without the ChimeraTK::Void type. */
  template<typename TargetType>
  class FixedUserTypeMapNoVoid {
   public:
    boost::fusion::map<boost::fusion::pair<int8_t, TargetType>, boost::fusion::pair<uint8_t, TargetType>,
        boost::fusion::pair<int16_t, TargetType>, boost::fusion::pair<uint16_t, TargetType>,
        boost::fusion::pair<int32_t, TargetType>, boost::fusion::pair<uint32_t, TargetType>,
        boost::fusion::pair<int64_t, TargetType>, boost::fusion::pair<uint64_t, TargetType>,
        boost::fusion::pair<float, TargetType>, boost::fusion::pair<double, TargetType>,
        boost::fusion::pair<std::string, TargetType>, boost::fusion::pair<Boolean, TargetType>>
        table;
  };

  /** Map of UserType to a class template with the UserType as template argument.
   * Used e.g. by the VirtualFunctionTemplate macros to implement the vtable. */
  template<template<typename> class TemplateClass>
  class TemplateUserTypeMap {
   public:
    boost::fusion::map<boost::fusion::pair<int8_t, TemplateClass<int8_t>>,
        boost::fusion::pair<uint8_t, TemplateClass<uint8_t>>, boost::fusion::pair<int16_t, TemplateClass<int16_t>>,
        boost::fusion::pair<uint16_t, TemplateClass<uint16_t>>, boost::fusion::pair<int32_t, TemplateClass<int32_t>>,
        boost::fusion::pair<uint32_t, TemplateClass<uint32_t>>, boost::fusion::pair<int64_t, TemplateClass<int64_t>>,
        boost::fusion::pair<uint64_t, TemplateClass<uint64_t>>, boost::fusion::pair<float, TemplateClass<float>>,
        boost::fusion::pair<double, TemplateClass<double>>,
        boost::fusion::pair<std::string, TemplateClass<std::string>>,
        boost::fusion::pair<Boolean, TemplateClass<Boolean>>, boost::fusion::pair<Void, TemplateClass<Void>>>
        table;
  };

  /** Just like TemplateUserTypeMap, only without the ChimeraTK::Void type. */
  template<template<typename> class TemplateClass>
  class TemplateUserTypeMapNoVoid {
   public:
    boost::fusion::map<boost::fusion::pair<int8_t, TemplateClass<int8_t>>,
        boost::fusion::pair<uint8_t, TemplateClass<uint8_t>>, boost::fusion::pair<int16_t, TemplateClass<int16_t>>,
        boost::fusion::pair<uint16_t, TemplateClass<uint16_t>>, boost::fusion::pair<int32_t, TemplateClass<int32_t>>,
        boost::fusion::pair<uint32_t, TemplateClass<uint32_t>>, boost::fusion::pair<int64_t, TemplateClass<int64_t>>,
        boost::fusion::pair<uint64_t, TemplateClass<uint64_t>>, boost::fusion::pair<float, TemplateClass<float>>,
        boost::fusion::pair<double, TemplateClass<double>>,
        boost::fusion::pair<std::string, TemplateClass<std::string>>,
        boost::fusion::pair<Boolean, TemplateClass<Boolean>>>
        table;
  };

  /** Map of UserType to a single type */
  template<typename T>
  using SingleTypeUserTypeMap = boost::fusion::map<boost::fusion::pair<int8_t, T>, boost::fusion::pair<uint8_t, T>,
      boost::fusion::pair<int16_t, T>, boost::fusion::pair<uint16_t, T>, boost::fusion::pair<int32_t, T>,
      boost::fusion::pair<uint32_t, T>, boost::fusion::pair<int64_t, T>, boost::fusion::pair<uint64_t, T>,
      boost::fusion::pair<float, T>, boost::fusion::pair<double, T>, boost::fusion::pair<std::string, T>,
      boost::fusion::pair<Boolean, T>, boost::fusion::pair<Void, T>>;

  /** Just like SingleTypeUserTypeMap, only without the ChimeraTK::Void type. */
  template<typename T>
  using SingleTypeUserTypeMapNoVoid = boost::fusion::map<boost::fusion::pair<int8_t, T>,
      boost::fusion::pair<uint8_t, T>, boost::fusion::pair<int16_t, T>, boost::fusion::pair<uint16_t, T>,
      boost::fusion::pair<int32_t, T>, boost::fusion::pair<uint32_t, T>, boost::fusion::pair<int64_t, T>,
      boost::fusion::pair<uint64_t, T>, boost::fusion::pair<float, T>, boost::fusion::pair<double, T>,
      boost::fusion::pair<std::string, T>, boost::fusion::pair<Boolean, T>>;

// Turn off the linter warning. It is wrong in this case. Putting parentheses would break the C++ syntax, the code does
// not compile
// NOLINTBEGIN(bugprone-macro-parentheses)
#define DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(TemplateClass)                                                       \
  extern template class TemplateClass<int8_t>;                                                                         \
  extern template class TemplateClass<uint8_t>;                                                                        \
  extern template class TemplateClass<int16_t>;                                                                        \
  extern template class TemplateClass<uint16_t>;                                                                       \
  extern template class TemplateClass<int32_t>;                                                                        \
  extern template class TemplateClass<uint32_t>;                                                                       \
  extern template class TemplateClass<int64_t>;                                                                        \
  extern template class TemplateClass<uint64_t>;                                                                       \
  extern template class TemplateClass<float>;                                                                          \
  extern template class TemplateClass<double>;                                                                         \
  extern template class TemplateClass<std::string>;                                                                    \
  extern template class TemplateClass<ChimeraTK::Boolean>;                                                             \
  extern template class TemplateClass<ChimeraTK::Void> // the last semicolon is added by the user

#define DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES_NO_VOID(TemplateClass)                                               \
  extern template class TemplateClass<int8_t>;                                                                         \
  extern template class TemplateClass<uint8_t>;                                                                        \
  extern template class TemplateClass<int16_t>;                                                                        \
  extern template class TemplateClass<uint16_t>;                                                                       \
  extern template class TemplateClass<int32_t>;                                                                        \
  extern template class TemplateClass<uint32_t>;                                                                       \
  extern template class TemplateClass<int64_t>;                                                                        \
  extern template class TemplateClass<uint64_t>;                                                                       \
  extern template class TemplateClass<float>;                                                                          \
  extern template class TemplateClass<double>;                                                                         \
  extern template class TemplateClass<std::string>;                                                                    \
  extern template class TemplateClass<ChimeraTK::Boolean> // the last semicolon is added by the user

#define INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(TemplateClass)                                                   \
  template class TemplateClass<int8_t>;                                                                                \
  template class TemplateClass<uint8_t>;                                                                               \
  template class TemplateClass<int16_t>;                                                                               \
  template class TemplateClass<uint16_t>;                                                                              \
  template class TemplateClass<int32_t>;                                                                               \
  template class TemplateClass<uint32_t>;                                                                              \
  template class TemplateClass<int64_t>;                                                                               \
  template class TemplateClass<uint64_t>;                                                                              \
  template class TemplateClass<float>;                                                                                 \
  template class TemplateClass<double>;                                                                                \
  template class TemplateClass<std::string>;                                                                           \
  template class TemplateClass<ChimeraTK::Boolean>;                                                                    \
  template class TemplateClass<ChimeraTK::Void> // the last semicolon is added by the user

#define INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES_NO_VOID(TemplateClass)                                           \
  template class TemplateClass<int8_t>;                                                                                \
  template class TemplateClass<uint8_t>;                                                                               \
  template class TemplateClass<int16_t>;                                                                               \
  template class TemplateClass<uint16_t>;                                                                              \
  template class TemplateClass<int32_t>;                                                                               \
  template class TemplateClass<uint32_t>;                                                                              \
  template class TemplateClass<int64_t>;                                                                               \
  template class TemplateClass<uint64_t>;                                                                              \
  template class TemplateClass<float>;                                                                                 \
  template class TemplateClass<double>;                                                                                \
  template class TemplateClass<std::string>;                                                                           \
  template class TemplateClass<ChimeraTK::Boolean> // the last semicolon is added by the user
// NOLINTEND(bugprone-macro-parentheses)

/** Macro to declare a template class with multiple template parameters for all
 *  supported user types. The variadic arguments are the additional template
 * parameters. Only works for classes where the user type is the first template
 * parameter.
 */
#define DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(TemplateClass, ...)                                            \
  extern template class TemplateClass<int8_t, __VA_ARGS__>;                                                            \
  extern template class TemplateClass<uint8_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<int16_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<uint16_t, __VA_ARGS__>;                                                          \
  extern template class TemplateClass<int32_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<uint32_t, __VA_ARGS__>;                                                          \
  extern template class TemplateClass<int64_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<uint64_t, __VA_ARGS__>;                                                          \
  extern template class TemplateClass<float, __VA_ARGS__>;                                                             \
  extern template class TemplateClass<double, __VA_ARGS__>;                                                            \
  extern template class TemplateClass<std::string, __VA_ARGS__>;                                                       \
  extern template class TemplateClass<ChimeraTK::Boolean, __VA_ARGS__>;                                                \
  extern template class TemplateClass<ChimeraTK::Void, __VA_ARGS__> // the last semicolon is added by the user

#define DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES_NO_VOID(TemplateClass, ...)                                    \
  extern template class TemplateClass<int8_t, __VA_ARGS__>;                                                            \
  extern template class TemplateClass<uint8_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<int16_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<uint16_t, __VA_ARGS__>;                                                          \
  extern template class TemplateClass<int32_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<uint32_t, __VA_ARGS__>;                                                          \
  extern template class TemplateClass<int64_t, __VA_ARGS__>;                                                           \
  extern template class TemplateClass<uint64_t, __VA_ARGS__>;                                                          \
  extern template class TemplateClass<float, __VA_ARGS__>;                                                             \
  extern template class TemplateClass<double, __VA_ARGS__>;                                                            \
  extern template class TemplateClass<std::string, __VA_ARGS__>;                                                       \
  extern template class TemplateClass<ChimeraTK::Boolean, __VA_ARGS__> // the last semicolon is added by the user

#define INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(TemplateClass, ...)                                        \
  template class TemplateClass<int8_t, __VA_ARGS__>;                                                                   \
  template class TemplateClass<uint8_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<int16_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<uint16_t, __VA_ARGS__>;                                                                 \
  template class TemplateClass<int32_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<uint32_t, __VA_ARGS__>;                                                                 \
  template class TemplateClass<int64_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<uint64_t, __VA_ARGS__>;                                                                 \
  template class TemplateClass<float, __VA_ARGS__>;                                                                    \
  template class TemplateClass<double, __VA_ARGS__>;                                                                   \
  template class TemplateClass<std::string, __VA_ARGS__>;                                                              \
  template class TemplateClass<ChimeraTK::Boolean, __VA_ARGS__>;                                                       \
  template class TemplateClass<ChimeraTK::Void, __VA_ARGS__> // the last semicolon is added by the user

#define INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES_NO_VOID(TemplateClass, ...)                                \
  template class TemplateClass<int8_t, __VA_ARGS__>;                                                                   \
  template class TemplateClass<uint8_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<int16_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<uint16_t, __VA_ARGS__>;                                                                 \
  template class TemplateClass<int32_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<uint32_t, __VA_ARGS__>;                                                                 \
  template class TemplateClass<int64_t, __VA_ARGS__>;                                                                  \
  template class TemplateClass<uint64_t, __VA_ARGS__>;                                                                 \
  template class TemplateClass<float, __VA_ARGS__>;                                                                    \
  template class TemplateClass<double, __VA_ARGS__>;                                                                   \
  template class TemplateClass<std::string, __VA_ARGS__>;                                                              \
  template class TemplateClass<ChimeraTK::Boolean, __VA_ARGS__> // the last semicolon is added by the user

  /** A class to describe which of the supported data types is used.
   *  There is the additional type 'none' to indicate that the data type is not
   * available in the current context. For instance if DataType is used to
   * identify the raw data type of an accessor, the value is none if the accessor
   * does not have a raw transfer mechanism.
   *
   *  The DataType behaves almost like a class enum due to the plain enum inside
   * it (see description of the <tt>operator TheType & ()</tt> ).
   */
  class DataType {
   public:
    /** The actual enum representing the data type. It is a plain enum so
     *  the data type class can be used like a class enum, i.e. types are
     *  identified for instance as DataType::int32.
     */
    enum TheType {
      none,    ///< The data type/concept does not exist, e.g. there is no raw transfer (do not confuse with Void)
      int8,    ///< Signed 8 bit integer
      uint8,   ///< Unsigned 8 bit integer
      int16,   ///< Signed 16 bit integer
      uint16,  ///< Unsigned 16 bit integer
      int32,   ///< Signed 32 bit integer
      uint32,  ///< Unsigned 32 bit integer
      int64,   ///< Signed 64 bit integer
      uint64,  ///< Unsigned 64 bit integer
      float32, ///< Single precision float
      float64, ///< Double precision float
      string,  ///< std::string
      Boolean, ///< Boolean
      Void     ///< Void
    };

    /** Implicit conversion operator to make DataType behave like a class enum.
     *  Allows for instance comparison or assigment with members of the TheType
     enum.
     *  \code
     DataType myDataType; // default constructor. The type now is 'none'.

     // DataType::int32 is of type DataType::TheType. The implicit conversion of
     \c myDataType
     // allows the assignment.
     myDataType = DataType::int32;
     \endcode
     */
    // Yes, this intentionally is an implicit conversion operator. Turn off the linter warning.
    // NOLINTBEGIN(google-explicit-constructor, hicpp-explicit-conversions)
    inline operator TheType&() { return _value; }
    inline operator TheType const&() const { return _value; }
    // NOLINTEND(google-explicit-constructor, hicpp-explicit-conversions)

    /** Return whether the raw data type is an integer.
     *  False is also returned for non-numerical types and 'none'.
     */
    [[nodiscard]] inline bool isIntegral() const {
      switch(_value) {
        case int8:
        case uint8:
        case int16:
        case uint16:
        case int32:
        case uint32:
        case int64:
        case uint64:
        case Boolean:
          return true;
        default:
          return false;
      }
    }

    /** Return whether the raw data type is signed. True for signed integers and
     *  floating point types (currently only signed implementations).
     *  False otherwise (also for non-numerical types and 'none').
     */
    [[nodiscard]] inline bool isSigned() const {
      switch(_value) {
        case int8:
        case int16:
        case int32:
        case int64:
        case float32:
        case float64:
          return true;
        default:
          return false;
      }
    }

    /** Returns whether the data type is numeric.
     *  Type 'none' returns false.
     */
    [[nodiscard]] inline bool isNumeric() const {
      // I inverted the logic to minimise the amout of code. If you add
      // non-numeric types this has to be adapted.
      switch(_value) {
        case none:
        case string:
        case Void:
          return false;
        default:
          return true;
      }
    }

    /** The constructor can get the type as an argument. It defaults to 'none'.
     */
    // We want implicit construction from the TheType enum.
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    inline DataType(TheType const& value = none) : _value(value) {}

    /** Construct DataType from std::type_info. If the type is not known, 'none' is returned. */
    // We want implicit construction from the type_info
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    inline DataType(const std::type_info& info) {
      if(info == typeid(int8_t)) {
        _value = int8;
      }
      else if(info == typeid(uint8_t)) {
        _value = uint8;
      }
      else if(info == typeid(int16_t)) {
        _value = int16;
      }
      else if(info == typeid(uint16_t)) {
        _value = uint16;
      }
      else if(info == typeid(int32_t)) {
        _value = int32;
      }
      else if(info == typeid(uint32_t)) {
        _value = uint32;
      }
      else if(info == typeid(int64_t)) {
        _value = int64;
      }
      else if(info == typeid(uint64_t)) {
        _value = uint64;
      }
      else if(info == typeid(float)) {
        _value = float32;
      }
      else if(info == typeid(double)) {
        _value = float64;
      }
      else if(info == typeid(std::string)) {
        _value = string;
      }
      else if(info == typeid(ChimeraTK::Boolean)) {
        _value = Boolean;
      }
      else if(info == typeid(ChimeraTK::Void)) {
        _value = Void;
      }
      else {
        _value = none;
      }
    }

    /** Construct DataType from std::string. */
    inline explicit DataType(const std::string& typeName) {
      if(typeName == "int8") {
        _value = int8;
      }
      else if(typeName == "uint8") {
        _value = uint8;
      }
      else if(typeName == "int16") {
        _value = int16;
      }
      else if(typeName == "uint16") {
        _value = uint16;
      }
      else if(typeName == "int32") {
        _value = int32;
      }
      else if(typeName == "uint32") {
        _value = uint32;
      }
      else if(typeName == "int64") {
        _value = int64;
      }
      else if(typeName == "uint64") {
        _value = uint64;
      }
      else if(typeName == "float32") {
        _value = float32;
      }
      else if(typeName == "float64") {
        _value = float64;
      }
      else if(typeName == "string") {
        _value = string;
      }
      else if(typeName == "Boolean") {
        _value = Boolean;
      }
      else if(typeName == "Void") {
        _value = Void;
      }
      else {
        _value = none;
      }
    }

    /** Return string representation of the data type */
    [[nodiscard]] inline std::string getAsString() const {
      switch(_value) {
        case int8:
          return "int8";
        case uint8:
          return "uint8";
        case int16:
          return "int16";
        case uint16:
          return "uint16";
        case int32:
          return "int32";
        case uint32:
          return "uint32";
        case int64:
          return "int64";
        case uint64:
          return "uint64";
        case float32:
          return "float32";
        case float64:
          return "float64";
        case string:
          return "string";
        case Boolean:
          return "Boolean";
        case Void:
          return "Void";
        default:
          return "unknown";
      }
    }

    /** Return std::type_info representation of the data type */
    [[nodiscard]] inline const std::type_info& getAsTypeInfo() const {
      switch(_value) {
        case int8:
          return typeid(int8_t);
        case uint8:
          return typeid(uint8_t);
        case int16:
          return typeid(int16_t);
        case uint16:
          return typeid(uint16_t);
        case int32:
          return typeid(int32_t);
        case uint32:
          return typeid(uint32_t);
        case int64:
          return typeid(int64_t);
        case uint64:
          return typeid(uint64_t);
        case float32:
          return typeid(float);
        case float64:
          return typeid(double);
        case string:
          return typeid(std::string);
        case Boolean:
          return typeid(ChimeraTK::Boolean);
        case Void:
          return typeid(ChimeraTK::Void);
        case none:
          return typeid(std::nullptr_t);
      }
    }

   protected:
    TheType _value;
  };

  /** Helper class for for_each(). */
  namespace detail {
    template<typename X>
    class for_each_callable {
     public:
      explicit for_each_callable(const X& fn) : fn_(fn) {}

      template<typename ARG_TYPE>
      void operator()(ARG_TYPE& argument) const {
        fn_(argument);
      }

     private:
      const X& fn_;
    };
  } // namespace detail

  /** Variant of boost::fusion::for_each() to iterate a boost::fusion::map, which
   * accepts a lambda instead of the callable class requred by the boost version.
   * The lambda must have one single argument of the type auto, which will be a
   * boost::fusion::pair<>. */
  template<typename MAPTYPE, typename LAMBDATYPE>
  void for_each(MAPTYPE& map, const LAMBDATYPE& lambda) {
    boost::fusion::for_each(map, detail::for_each_callable<LAMBDATYPE>(lambda));
  }

  /**
   *  Helper function for running code which uses some compile-time type that is
   * specified at runtime as a type_info. The code has to be written in a lambda
   * with a single auto-typed argument (this requires C++ 14). This argument will
   * have the type specified by the argument "type". The type must be one of the
   * user types supported by ChimeraTK DeviceAccess. Otherwise, std::bad_cast is
   * thrown.
   *
   *  The lamda should be declared like this:
   *
   *    auto myLambda [](auto arg) { std::cout << typeid(decltype(arg)).name() <<
   * std::endl; });
   *
   *  The value of the argument "arg" is undefined, only its type should be used
   * (via decltype(arg)). This lambda can then be called like this:
   *
   *    callForType(typeid(int), myLambda);
   *
   *  This will effectively execute the following code:
   *
   *    std::cout << typeid(int).name() << std::endl;
   *
   *  Please note that this call is not as efficient as a direct call to a
   * template. For each call all allowed user types have to be tested against the
   * given type_info.
   */
  template<typename LAMBDATYPE>
  void callForType(const std::type_info& type, LAMBDATYPE lambda) {
    bool done = false;

    // iterate through a userTypeMap() and test all types
    for_each(userTypeMap(), [&type, &lambda, &done](auto pair) {
      if(type != typeid(pair.second)) return;
      // if the right type has been found, call the original lambda
      lambda(pair.second);
      done = true;
    });

    // Check if done flag has been set. If not, an unknown type has been passed.
    if(!done) {
      class myBadCast : public std::bad_cast {
       public:
        explicit myBadCast(std::string desc) : _desc(std::move(desc)) {}
        [[nodiscard]] const char* what() const noexcept override { return _desc.c_str(); }

       private:
        std::string _desc;
      };
      throw myBadCast(std::string("ChimeraTK::callForType(): type is not known: ") + type.name());
    }
  }

  /**
   *  Alternate form of callForType() which takes a DataType as a runtime type
   * description. If DataType::none is passed, std::bad_cast is thrown. For more
   * details have a look at the other form.
   */
  template<typename LAMBDATYPE>
  void callForType(const DataType& type, LAMBDATYPE lambda) {
    switch(DataType::TheType(type)) {
      // The linter thinks the branches are identical. They are not, they have a different type, which is the whole
      // point of this exercise.
      // NOLINTBEGIN(bugprone-branch-clone)
      case DataType::int8: {
        lambda(int8_t());
      } break;
      case DataType::uint8: {
        lambda(uint8_t());
      } break;
      case DataType::int16: {
        lambda(int16_t());
      } break;
      case DataType::uint16: {
        lambda(uint16_t());
      } break;
      case DataType::int32: {
        lambda(int32_t());
      } break;
      case DataType::uint32: {
        lambda(uint32_t());
      } break;
      case DataType::int64: {
        lambda(int64_t());
      } break;
      case DataType::uint64: {
        lambda(uint64_t());
      } break;
      case DataType::float32: {
        lambda(float());
      } break;
      case DataType::float64: {
        lambda(double());
      } break;
      case DataType::string: {
        lambda(std::string());
      } break;
      case DataType::Boolean: {
        lambda(Boolean());
      } break;
      case DataType::Void: {
        lambda(Void());
      } break;
      // NOLINTEND(bugprone-branch-clone)
      case DataType::none:
        class myBadCast : public std::bad_cast {
         public:
          explicit myBadCast(std::string desc) : _desc(std::move(desc)) {}
          [[nodiscard]] const char* what() const noexcept override { return _desc.c_str(); }

         private:
          std::string _desc;
        };
        throw myBadCast(std::string("ChimeraTK::callForType() has been called for DataType::none"));
    }
  }

  /** Like callForType() but omit the Void data type. typeDescriptor can be either a std::type_info or a
   *  ChimeraTK::DataType. */
  template<typename TYPE_DESCRIPTOR, typename LAMBDATYPE>
  void callForTypeNoVoid(const TYPE_DESCRIPTOR& typeDescriptor, LAMBDATYPE lambda) {
    callForType(typeDescriptor, [lambda](auto arg) {
      if constexpr(!std::is_same<decltype(arg), Void>::value) {
        lambda(arg);
      }
    });
  }

  /**
   *  callForRawType() is similar to callForType(), just with a subset of supported data types which can be raw types
   *  in the NumericAddressedBackend.
   */
  template<typename LAMBDATYPE>
  void callForRawType(const DataType& type, LAMBDATYPE lambda) {
    switch(DataType::TheType(type)) {
      // The linter thinks the branches are identical. They are not, they have a different type, which is the whole
      // point of this exercise.
      // NOLINTBEGIN(bugprone-branch-clone)
      case DataType::Void: {
        // do nothing:
      } break;
      case DataType::int8: {
        lambda(int8_t());
      } break;
      case DataType::int16: {
        lambda(int16_t());
      } break;
      case DataType::int32: {
        lambda(int32_t());
      } break;
      // NOLINTEND(bugprone-branch-clone)
      default:
        class myBadCast : public std::bad_cast {
         public:
          explicit myBadCast(std::string desc) : _desc(std::move(desc)) {}
          [[nodiscard]] const char* what() const noexcept override { return _desc.c_str(); }

         private:
          std::string _desc;
        };
        throw myBadCast(std::string("ChimeraTK::callForType() has been called for DataType::none"));
    }
  }

} /* namespace ChimeraTK */
