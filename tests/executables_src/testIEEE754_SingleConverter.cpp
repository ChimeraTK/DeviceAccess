// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE IEEE754_SingleConverterTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Exception.h"
#include "IEEE754_SingleConverter.h"
using namespace ChimeraTK;

#include <float.h> // for float limits

BOOST_AUTO_TEST_CASE(test_toCooked_3_25) {
  IEEE754_SingleConverter converter;

  float testValue = 3.25;
  int32_t rawValue;
  memcpy(&rawValue, &testValue, sizeof(float));

  BOOST_CHECK_CLOSE(converter.scalarToCooked<float>(rawValue), 3.25, 0.0001);
  BOOST_CHECK_CLOSE(converter.scalarToCooked<double>(rawValue), 3.25, 0.0001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int8_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint8_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int16_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint16_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int32_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint32_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int64_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint64_t>(rawValue), 3);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<std::string>(rawValue), std::to_string(testValue));
  BOOST_CHECK_EQUAL(converter.scalarToCooked<Boolean>(rawValue), true);
}

BOOST_AUTO_TEST_CASE(test_toCooked_60k) {
  IEEE754_SingleConverter converter;

  // tests two functionalities: range check of the target (value too large for
  // int8 and int16)
  float testValue = 60000.7;
  int32_t rawValue;
  memcpy(&rawValue, &testValue, sizeof(float));

  BOOST_CHECK_CLOSE(converter.scalarToCooked<float>(rawValue), 60000.7, 0.0001);
  BOOST_CHECK_CLOSE(converter.scalarToCooked<double>(rawValue), 60000.7, 0.0001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int8_t>(rawValue), 127);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint8_t>(rawValue), 255);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int16_t>(rawValue), 32767);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint16_t>(rawValue), 60001); // unsigned 16 bit is up to 65k
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int32_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint32_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int64_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint64_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<std::string>(rawValue), std::to_string(testValue));
  BOOST_CHECK_EQUAL(converter.scalarToCooked<Boolean>(rawValue), true);
}

BOOST_AUTO_TEST_CASE(test_toCooked_minus240) {
  IEEE754_SingleConverter converter;

  float testValue = -240.6;
  int32_t rawValue;
  memcpy(&rawValue, &testValue, sizeof(float));

  BOOST_CHECK_CLOSE(converter.scalarToCooked<float>(rawValue), -240.6, 0.0001);
  BOOST_CHECK_CLOSE(converter.scalarToCooked<double>(rawValue), -240.6, 0.0001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int8_t>(rawValue), -128);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint8_t>(rawValue), 0);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int16_t>(rawValue), -241);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint16_t>(rawValue), 0);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int32_t>(rawValue), -241);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint32_t>(rawValue), 0);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int64_t>(rawValue), -241);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint64_t>(rawValue), 0);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<std::string>(rawValue), std::to_string(testValue));
  BOOST_CHECK_EQUAL(converter.scalarToCooked<Boolean>(rawValue), true);
}

void checkAsRaw(int32_t rawValue, float expectedValue) {
  float testValue;
  memcpy(&testValue, &rawValue, sizeof(float));

  BOOST_CHECK_CLOSE(testValue, expectedValue, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_from_3_25) {
  IEEE754_SingleConverter converter;

  checkAsRaw(converter.toRaw(float(3.25)), 3.25);
  checkAsRaw(converter.toRaw(double(-3.25)), -3.25);
  checkAsRaw(converter.toRaw(int8_t(-3)), -3);
  checkAsRaw(converter.toRaw(uint8_t(3)), 3);
  checkAsRaw(converter.toRaw(int16_t(-3)), -3);
  checkAsRaw(converter.toRaw(uint16_t(3)), 3);
  checkAsRaw(converter.toRaw(int32_t(3)), 3);
  checkAsRaw(converter.toRaw(uint32_t(3)), 3);
  checkAsRaw(converter.toRaw(int64_t(3)), 3);
  checkAsRaw(converter.toRaw(uint64_t(3)), 3);
  checkAsRaw(converter.toRaw(std::string("3.25")), 3.25);
  checkAsRaw(converter.toRaw(Boolean("3.25")), true);

  // corner cases
  BOOST_CHECK_THROW(std::ignore = converter.toRaw(std::string("notAFloat")), ChimeraTK::logic_error);

  double tooLarge = DBL_MAX + 1;
  double tooSmall = -(DBL_MAX);

  // converter should limit, not throw
  checkAsRaw(converter.toRaw(tooLarge), FLT_MAX);
  checkAsRaw(converter.toRaw(tooSmall), -FLT_MAX);
}

BOOST_AUTO_TEST_CASE(test_toCooked_00) {
  IEEE754_SingleConverter converter;

  // tests if Boolean turns 0.0 into false
  float testValue = 0.0;
  int32_t rawValue;
  memcpy(&rawValue, &testValue, sizeof(float));

  BOOST_CHECK_EQUAL(converter.scalarToCooked<Boolean>(rawValue), false);
}
