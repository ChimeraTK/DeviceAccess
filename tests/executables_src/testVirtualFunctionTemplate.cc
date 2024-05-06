// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE VirtualFunctionTemplateTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "SupportedUserTypes.h"
#include "VirtualFunctionTemplate.h"

namespace ChimeraTK {
class Base {
 public:
  Base() { FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getValue_impl);
  }
  template <typename T>
  std::string getValue(T& base) { return CALL_VIRTUAL_FUNCTION_TEMPLATE(getValue_impl, T, base); }

  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getValue_impl, std::string(T&));

 private:
  template<typename T>
  std::string getValue_impl(T& /*base*/) {
    return std::string("Base: ") + typeid(T).name();
  }
};

class Derived1 : public Base {
 public:
  Derived1() { OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(Base, getValue_impl); }
  DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE(Base, getValue_impl, std::string(T&));

 private:
  template<typename T>
  std::string getValue_impl(T& base) {
    if constexpr(std::is_same_v<std::string, T>) {
      return CALL_BASE_FUNCTION_TEMPLATE(Base, getValue_impl, T, base);
    }
    return std::string("Derived1: ") + typeid(T).name();
  }
};

class Derived2 : public Derived1 {
 public:
  Derived2() { OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(Derived1, getValue_impl); }
  DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE(Derived1, getValue_impl, std::string(T&));

 private:
  template<typename T>
  std::string getValue_impl(T& base) {
    if constexpr(std::is_same_v<float, T>) {
      return CALL_BASE_FUNCTION_TEMPLATE(Derived1, getValue_impl, T, base);
    }
    else if constexpr(std::is_same_v<double, T>) {
      return CALL_BASE_FUNCTION_TEMPLATE(Base, getValue_impl, T, base);
    }
    return std::string("Derived2: ") + typeid(T).name();
  }
};

} // namespace ChimeraTK

BOOST_AUTO_TEST_CASE(testBaseClass) {
  ChimeraTK::Base base;
  ChimeraTK::userTypeMap typeMap;

  ChimeraTK::for_each(typeMap, [&](auto& pair) {
    typedef typename std::remove_reference<decltype(pair)>::type::first_type T;
    T argument;
    BOOST_TEST(base.getValue<T>(argument) == std::string("Base: ") + typeid(T).name());
  });
}

BOOST_AUTO_TEST_CASE(testDerived1Class) {
  ChimeraTK::Derived1 base;
  ChimeraTK::userTypeMap typeMap;

  ChimeraTK::for_each(typeMap, [&](auto& pair) {
    typedef typename std::remove_reference<decltype(pair)>::type::first_type T;
    T argument;
    if constexpr(std::is_same_v<T, std::string>) {
      BOOST_TEST(base.getValue<T>(argument) == std::string("Base: ") + typeid(T).name());
    } else {
      BOOST_TEST(base.getValue<T>(argument) == std::string("Derived1: ") + typeid(T).name());
    }
  });
}

BOOST_AUTO_TEST_CASE(testDerived2Class) {
  ChimeraTK::Derived2 base;
  ChimeraTK::userTypeMap typeMap;

  ChimeraTK::for_each(typeMap, [&](auto& pair) {
    typedef typename std::remove_reference<decltype(pair)>::type::first_type T;
    T argument;
    if constexpr(std::is_same_v<T, double>) {
      BOOST_TEST(base.getValue<T>(argument) == std::string("Base: ") + typeid(T).name());
    } else if constexpr(std::is_same_v<T, float>) {
      BOOST_TEST(base.getValue<T>(argument) == std::string("Derived1: ") + typeid(T).name());
    } else {
      BOOST_TEST(base.getValue<T>(argument) == std::string("Derived2: ") + typeid(T).name());
    }
  });
}
