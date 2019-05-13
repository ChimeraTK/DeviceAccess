/*
 * SupportedUserTypes.h - Define boost::fusion::maps of the user data types
 * supported by the CHIMERA_TK library.
 *
 *  Created on: Feb 29, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_SUPPORTED_USER_TYPES_H
#define CHIMERA_TK_SUPPORTED_USER_TYPES_H

#include <iterator>
#include <sstream>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace ChimeraTK {

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

    template<class S>
    struct Round {
      static S nearbyint(S s) { return std::round(s); }
      typedef boost::mpl::integral_c<std::float_round_style, std::round_to_nearest> round_style;
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
    return converter::convert(value);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename NUMERIC>
  NUMERIC detail::userTypeToNumeric_impl<UserType, NUMERIC>::impl(UserType value) {
    typedef boost::numeric::converter<NUMERIC, UserType, boost::numeric::conversion_traits<NUMERIC, UserType>,
        boost::numeric::def_overflow_handler, Round<UserType>>
        converter;
    return converter::convert(value);
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
    return std::to_string(value);
  }

  /********************************************************************************************************************/

  template<typename NUMERIC>
  NUMERIC detail::userTypeToNumeric_impl<std::string, NUMERIC>::impl(const std::string& value) {
    NUMERIC v;
    std::stringstream ss(value);
    ss >> v;
    return v;
  }

  /********************************************************************************************************************/

  /** Helper function to convert any UserType data into any other UserType (even if it is a string etc.). The conversion
   *  is done with proper rounding and range checking for numeric types. It will throw boost::numeric::positive_overflow
   *  resp. boost::numeric::negative_overflow if the data is out of range. */
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

  /** A random-access iterator which can be created from raw C pointers */
  template<typename DATA_TYPE>
  struct raw_iterator : std::iterator<std::random_access_iterator_tag, DATA_TYPE> {
    explicit raw_iterator(DATA_TYPE* begin) : _ptr(begin) {}
    raw_iterator& operator++() {
      ++_ptr;
      return *this;
    }
    raw_iterator operator++(int) {
      raw_iterator retval = *this;
      ++(*this);
      return retval;
    }
    bool operator==(raw_iterator other) const { return _ptr == other._ptr; }
    bool operator!=(raw_iterator other) const { return !(*this == other); }
    size_t operator-(raw_iterator other) const { return _ptr - other._ptr; }
    DATA_TYPE& operator*() const { return *_ptr; }

   private:
    DATA_TYPE* _ptr;
  };

  /********************************************************************************************************************/

  /** Map of UserType to value of the UserType. Used e.g. by the FixedPointConverter to store coefficients etc. in
   *  dependence of the UserType. */
  typedef boost::fusion::map<boost::fusion::pair<int8_t, int8_t>, boost::fusion::pair<uint8_t, uint8_t>,
      boost::fusion::pair<int16_t, int16_t>, boost::fusion::pair<uint16_t, uint16_t>,
      boost::fusion::pair<int32_t, int32_t>, boost::fusion::pair<uint32_t, uint32_t>,
      boost::fusion::pair<int64_t, int64_t>, boost::fusion::pair<uint64_t, uint64_t>, boost::fusion::pair<float, float>,
      boost::fusion::pair<double, double>, boost::fusion::pair<std::string, std::string>>
      userTypeMap;

  /** Map of UserType to a value of a single type (same for evey user type) */
  template<typename TargetType>
  class FixedUserTypeMap {
   public:
    boost::fusion::map<boost::fusion::pair<int8_t, TargetType>, boost::fusion::pair<uint8_t, TargetType>,
        boost::fusion::pair<int16_t, TargetType>, boost::fusion::pair<uint16_t, TargetType>,
        boost::fusion::pair<int32_t, TargetType>, boost::fusion::pair<uint32_t, TargetType>,
        boost::fusion::pair<int64_t, TargetType>, boost::fusion::pair<uint64_t, TargetType>,
        boost::fusion::pair<float, TargetType>, boost::fusion::pair<double, TargetType>,
        boost::fusion::pair<std::string, TargetType>>
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
        boost::fusion::pair<std::string, TemplateClass<std::string>>>
        table;
  };

  /** Map of UserType to a single type */
  template<typename T>
  using SingleTypeUserTypeMap = boost::fusion::map<boost::fusion::pair<int8_t, T>, boost::fusion::pair<uint8_t, T>,
      boost::fusion::pair<int16_t, T>, boost::fusion::pair<uint16_t, T>, boost::fusion::pair<int32_t, T>,
      boost::fusion::pair<uint32_t, T>, boost::fusion::pair<int64_t, T>, boost::fusion::pair<uint64_t, T>,
      boost::fusion::pair<float, T>, boost::fusion::pair<double, T>, boost::fusion::pair<std::string, T>>;

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
  extern template class TemplateClass<std::string> // the last semicolon is added by the user

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
  template class TemplateClass<std::string> // the last semicolon is added by the user

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
  extern template class TemplateClass<std::string, __VA_ARGS__> // the last semicolon is added by the user

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
  template class TemplateClass<std::string, __VA_ARGS__> // the last semicolon is added by the user

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
      none,    ///< The data type/concept does not exist. e.g. there is no raw
               ///< transfer
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
      string   ///< std::string
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
    inline operator TheType&() { return _value; }
    inline operator TheType const&() const { return _value; }

    /** Return whether the raw data type is an integer.
     *  False is also returned for non-numerical types and 'none'.
     */
    inline bool isIntegral() const {
      switch(_value) {
        case int8:
        case uint8:
        case int16:
        case uint16:
        case int32:
        case uint32:
        case int64:
        case uint64:
          return true;
        default:
          return false;
      }
    }

    /** Return whether the raw data type is signed. True for signed integers and
     *  floating point types (currently only signed implementations).
     *  False otherwise (also for non-numerical types and 'none').
     */
    inline bool isSigned() const {
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
    inline bool isNumeric() const {
      // I inverted the logic to minimise the amout of code. If you add
      // non-numeric types this has to be adapted.
      switch(_value) {
        case none:
        case string:
          return false;
        default:
          return true;
      }
    }

    /** The constructor can get the type as an argument. It defaults to 'none'.
     */
    inline DataType(TheType const& value = none) : _value(value) {}

    /** Construct DataType from std::type_info. If the type is not known, 'none' is returned. */
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
      else {
        _value = none;
      }
    }

    /** Construct DataType from std::string. */
    inline DataType(const std::string& typeName) {
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
      else {
        _value = none;
      }
    }

    /** Return string representation of the data type */
    inline bool getAsString() const {
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
          return "in64";
        case uint64:
          return "uin64";
        case float32:
          return "float32";
        case float64:
          return "float64";
        case string:
          return "string";
        default:
          return false;
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
      for_each_callable(X& fn) : fn_(fn) {}

      template<typename ARG_TYPE>
      void operator()(ARG_TYPE& argument) const {
        fn_(argument);
      }

     private:
      X& fn_;
    };
  } // namespace detail

  /** Variant of boost::fusion::for_each() to iterate a boost::fusion::map, which
   * accepts a lambda instead of the callable class requred by the boost version.
   * The lambda must have one single argument of the type auto, which will be a
   * boost::fusion::pair<>. */
  template<typename MAPTYPE, typename LAMBDATYPE>
  void for_each(MAPTYPE& map, LAMBDATYPE& lambda) {
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
        myBadCast(const std::string& desc) : _desc(desc) {}
        const char* what() const noexcept override { return _desc.c_str(); }

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
      case DataType::none:
        class myBadCast : public std::bad_cast {
         public:
          myBadCast(const std::string& desc) : _desc(desc) {}
          const char* what() const noexcept override { return _desc.c_str(); }

         private:
          std::string _desc;
        };
        throw myBadCast(std::string("ChimeraTK::callForType() has been called for DataType::none"));
    }
  }

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_SUPPORTED_USER_TYPES_H */
