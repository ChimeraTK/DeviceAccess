#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE IEEE754_SingleConverterTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "IEEE754_SingleConverter.h"
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE( test_3_25 ){
  IEEE754_SingleConverter converter;

  float testValue = 3.25;
  void * warningAvoider = &testValue;
  int32_t rawValue = *(reinterpret_cast<int32_t *>(warningAvoider));

  BOOST_CHECK_CLOSE( converter.toCooked<float>(rawValue), 3.25, 0.0001);
  BOOST_CHECK_CLOSE( converter.toCooked<double>(rawValue), 3.25, 0.0001);
  BOOST_CHECK_EQUAL( converter.toCooked<int8_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<uint8_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<int16_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<uint16_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<int32_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<uint32_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<int64_t>(rawValue), 3);
  BOOST_CHECK_EQUAL( converter.toCooked<uint64_t>(rawValue), 3);
  //BOOST_CHECK_EQUAL( converter.toCooked<std::string>(rawValue), std::to_string(testValue));
}

BOOST_AUTO_TEST_CASE( test_60k ){
  IEEE754_SingleConverter converter;

  // tests two functionalities: range check of the target (value too large for int8 and int16)
  float testValue = 60000.7;
  void * warningAvoider = &testValue;
  int32_t rawValue = *(reinterpret_cast<int32_t *>(warningAvoider));

  BOOST_CHECK_CLOSE( converter.toCooked<float>(rawValue), 60000.7, 0.0001);
  BOOST_CHECK_CLOSE( converter.toCooked<double>(rawValue), 60000.7, 0.0001);
  BOOST_CHECK_THROW( converter.toCooked<int8_t>(rawValue), ChimeraTK::runtime_error);
  BOOST_CHECK_THROW( converter.toCooked<uint8_t>(rawValue), ChimeraTK::runtime_error);
  BOOST_CHECK_THROW( converter.toCooked<int16_t>(rawValue), ChimeraTK::runtime_error); // +- 32k
  BOOST_CHECK_EQUAL( converter.toCooked<uint16_t>(rawValue), 60001); // unsigned 16 bit is up to 65k
  BOOST_CHECK_EQUAL( converter.toCooked<int32_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL( converter.toCooked<uint32_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL( converter.toCooked<int64_t>(rawValue), 60001);
  BOOST_CHECK_EQUAL( converter.toCooked<uint64_t>(rawValue), 60001);
  //BOOST_CHECK_EQUAL( converter.toCooked<std::string>(rawValue), std::to_string(testValue);
}

BOOST_AUTO_TEST_CASE( test_minus240 ){
  IEEE754_SingleConverter converter;

  float testValue = -240.6;
  void * warningAvoider = &testValue;
  int32_t rawValue = *(reinterpret_cast<int32_t *>(warningAvoider));

  BOOST_CHECK_CLOSE( converter.toCooked<float>(rawValue), -240.6, 0.0001);
  BOOST_CHECK_CLOSE( converter.toCooked<double>(rawValue), -240.6, 0.0001);
  BOOST_CHECK_THROW( converter.toCooked<int8_t>(rawValue), ChimeraTK::runtime_error);
  BOOST_CHECK_THROW( converter.toCooked<uint8_t>(rawValue), ChimeraTK::runtime_error);
  BOOST_CHECK_EQUAL( converter.toCooked<int16_t>(rawValue), -241);
  BOOST_CHECK_THROW( converter.toCooked<uint16_t>(rawValue), ChimeraTK::runtime_error);
  BOOST_CHECK_EQUAL( converter.toCooked<int32_t>(rawValue), -241);
  BOOST_CHECK_THROW( converter.toCooked<uint32_t>(rawValue), ChimeraTK::runtime_error);
  BOOST_CHECK_EQUAL( converter.toCooked<int64_t>(rawValue), -241);
  BOOST_CHECK_THROW( converter.toCooked<uint64_t>(rawValue), ChimeraTK::runtime_error);
  //BOOST_CHECK_EQUAL( converter.toCooked<std::string>(rawValue), std::to_string(testValue));
}
