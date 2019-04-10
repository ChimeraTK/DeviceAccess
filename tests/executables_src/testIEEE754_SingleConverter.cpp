#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE IEEE754_SingleConverterTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "IEEE754_SingleConverter.h"
using namespace ChimeraTK;

#include <float.h> // for float limits

BOOST_AUTO_TEST_CASE(test_toCooked_3_25) {
  IEEE754_SingleConverter converter;

  float testValue = 3.25;
  void* warningAvoider = &testValue;
  int32_t rawValue = *(reinterpret_cast<int32_t*>(warningAvoider));

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
}

BOOST_AUTO_TEST_CASE(test_toCooked_60k) {
  IEEE754_SingleConverter converter;

  // tests two functionalities: range check of the target (value too large for
  // int8 and int16)
  float testValue = 60000.7;
  void* warningAvoider = &testValue;
  int32_t rawValue = *(reinterpret_cast<int32_t*>(warningAvoider));

  BOOST_CHECK_CLOSE(converter.scalarToCooked<float>(rawValue), 60000.7, 0.0001);
  BOOST_CHECK_CLOSE(converter.scalarToCooked<double>(rawValue), 60000.7, 0.0001);
  BOOST_CHECK_THROW(converter.scalarToCooked<int8_t>(rawValue), boost::numeric::positive_overflow);
  BOOST_CHECK_THROW(converter.scalarToCooked<uint8_t>(rawValue), boost::numeric::positive_overflow);
  BOOST_CHECK_THROW(converter.scalarToCooked<int16_t>(rawValue), boost::numeric::positive_overflow); // +- 32k
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint16_t>(rawValue), 60001); // unsigned 16 bit is up to 65k
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int32_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint32_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int64_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<uint64_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<std::string>(rawValue), std::to_string(testValue));
}

BOOST_AUTO_TEST_CASE(test_toCooked_minus240) {
  IEEE754_SingleConverter converter;

  float testValue = -240.6;
  void* warningAvoider = &testValue;
  int32_t rawValue = *(reinterpret_cast<int32_t*>(warningAvoider));

  BOOST_CHECK_CLOSE(converter.scalarToCooked<float>(rawValue), -240.6, 0.0001);
  BOOST_CHECK_CLOSE(converter.scalarToCooked<double>(rawValue), -240.6, 0.0001);
  BOOST_CHECK_THROW(converter.scalarToCooked<int8_t>(rawValue), boost::numeric::negative_overflow);
  BOOST_CHECK_THROW(converter.scalarToCooked<uint8_t>(rawValue), boost::numeric::negative_overflow);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int16_t>(rawValue), -241);
  BOOST_CHECK_THROW(converter.scalarToCooked<uint16_t>(rawValue), boost::numeric::negative_overflow);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int32_t>(rawValue), -241);
  BOOST_CHECK_THROW(converter.scalarToCooked<uint32_t>(rawValue), boost::numeric::negative_overflow);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<int64_t>(rawValue), -241);
  BOOST_CHECK_THROW(converter.scalarToCooked<uint64_t>(rawValue), boost::numeric::negative_overflow);
  BOOST_CHECK_EQUAL(converter.scalarToCooked<std::string>(rawValue), std::to_string(testValue));
}

void checkAsRaw(int32_t rawValue, float expectedValue) {
  void* warningAvoider = &rawValue;
  float testValue = *(reinterpret_cast<float*>(warningAvoider));

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

  // corner cases
  BOOST_CHECK_THROW(converter.toRaw(std::string("notAFloat")), ChimeraTK::logic_error);

  double tooLarge = DBL_MAX + 1;
  double tooSmall = -(DBL_MAX);

  // converter should limit, not throw
  checkAsRaw(converter.toRaw(tooLarge), FLT_MAX);
  checkAsRaw(converter.toRaw(tooSmall), -FLT_MAX);
}
