#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FixedPointConverterTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

/*  Test requirements:
 *  Test to and from double for the follwing cases:
 *  int32, uint32, int16, uint16, int8, uint8. No fractional bits (standard data
 * types) 32 bits with -12 (negative), -1 (test rounding), 1 (test rounding), 7
 * (somewhere in the middle), 31, 32 (resolution edge) and 43 (larger than 32
 * bits), fractional bits, signed and unsigned 18 bits with -12, 0, 7, 17, 18,
 * 43 fractional bits, signed and unsigned
 *
 *  All tests are run with the bit sequence 0xAAAAAAAA (negative when signed)
 *  and 0x55555555 (positive when signed) to float,
 *  and with +-0.25, +-0.75, +-3.25 +-5.75 to fixed
 *  to test correct rounding.
 *
 */

#define HEX_TO_DOUBLE(INPUT) static_cast<double>(INPUT)
#define SIGNED_HEX_TO_DOUBLE(INPUT) static_cast<double>(static_cast<int32_t>(INPUT))
#define SIGNED_HEX_TO_INT64(INPUT) static_cast<int64_t>(static_cast<int32_t>(INPUT))

#define CHECK_SIGNED_FIXED_POINT_NO_FRACTIONAL                                                                         \
  checkToFixedPoint(converter, 0.25, 0);                                                                               \
  checkToFixedPoint(converter, -0.25, 0);                                                                              \
  checkToFixedPoint(converter, 0.75, 1);                                                                               \
  checkToFixedPoint(converter, -0.75, -1);                                                                             \
  checkToFixedPoint(converter, 3.25, 3);                                                                               \
  checkToFixedPoint(converter, -3.25, -3);                                                                             \
  checkToFixedPoint(converter, 5.75, 6);                                                                               \
  checkToFixedPoint(converter, -5.75, -6);

#include "Exception.h"
#include <sstream>

#include "FixedPointConverter.h"
namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

// get name of a type for better readible error messages
template<typename T>
inline const char* typeName(void) {
  return "unknown";
}
#define DEF_TYPENAME(name)                                                                                             \
  template<>                                                                                                           \
  inline const char* typeName<name>(void) {                                                                            \
    return #name;                                                                                                      \
  }
DEF_TYPENAME(int32_t)
DEF_TYPENAME(uint32_t)
DEF_TYPENAME(int16_t)
DEF_TYPENAME(uint16_t)
DEF_TYPENAME(int8_t)
DEF_TYPENAME(uint8_t)
DEF_TYPENAME(int64_t)
DEF_TYPENAME(uint64_t)
DEF_TYPENAME(float)
DEF_TYPENAME(double)
DEF_TYPENAME(std::string)

template<typename T>
void checkToCookedOverflowNeg(FixedPointConverter const& converter, uint32_t input) {
  std::stringstream message;
  message << "checkToCookedOverflowNeg failed for type " << typeName<T>();

  bool hasThrown = false;
  try {
    converter.scalarToCooked<T>(input);
  }
  catch(boost::numeric::negative_overflow& e) {
    // Although we do not check the content of the what() message, we execute it
    // here to see that it's working. We leave the printout in here to have the
    // code covarage and the possibility to manually check the output of the
    // test.
    std::cout << "Caught exception where it was expected: " << e.what() << std::endl;
    hasThrown = true;
  }
  if(!hasThrown) BOOST_FAIL(message.str());
}

template<typename T>
void checkToCookedOverflowPos(FixedPointConverter const& converter, uint32_t input) {
  std::stringstream message;
  message << "checkToCookedOverflowPos failed for type " << typeName<T>();

  bool hasThrown = false;
  try {
    converter.scalarToCooked<T>(input);
  }
  catch(boost::numeric::positive_overflow& e) {
    std::cout << "Caught exception where it was expected: " << e.what() << std::endl;
    hasThrown = true;
  }
  if(!hasThrown) BOOST_FAIL(message.str());
}

template<typename T>
void checkToCooked(FixedPointConverter const& converter, uint32_t input, T expectedValue) {
  std::stringstream message;
  message << "testToCooked failed for type " << typeName<T>() << " with input " << std::hex << "0x" << input << std::hex
          << ", expected 0x" << expectedValue << std::dec;

  BOOST_TEST_CHECKPOINT(message.str());

  T output = converter.scalarToCooked<T>(input);

  message << std::hex << ", output 0x" << output << std::dec;

  BOOST_CHECK_MESSAGE(output == expectedValue, message.str());
}

template<typename T>
void checkToRaw(FixedPointConverter const& converter, T input, uint32_t expectedValue) {
  std::stringstream message;
  message << "testToRaw failed for type " << typeName<T>() << " with input 0x" << std::hex << input << ", expected 0x"
          << expectedValue << std::dec;

  BOOST_TEST_CHECKPOINT(message.str());

  uint32_t result = converter.toRaw(input);

  message << std::hex << ", output 0x" << result << std::dec;

  BOOST_CHECK_MESSAGE(result == expectedValue, message.str());
}

BOOST_AUTO_TEST_SUITE(FixedPointConverterTestSuite)

BOOST_AUTO_TEST_CASE(testConstructor) {
  BOOST_CHECK_NO_THROW(FixedPointConverter("UnknownVariable"));
  BOOST_CHECK_NO_THROW(FixedPointConverter("UnknownVariable", 16, 42, false));

  // number of significant bits
  BOOST_CHECK_THROW(FixedPointConverter("UnknownVariable", 33), ChimeraTK::logic_error);

  // dynamic range of sufficient for bit shift
  BOOST_CHECK_THROW(FixedPointConverter("UnknownVariable", 2, 1021 - 1), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(FixedPointConverter("UnknownVariable", 2, -1024 + 1), ChimeraTK::logic_error);
  BOOST_CHECK_NO_THROW(FixedPointConverter("UnknownVariable", 2, 1021 - 2));
  BOOST_CHECK_NO_THROW(FixedPointConverter("UnknownVariable", 2, -1024 + 2));
}

BOOST_AUTO_TEST_CASE(testInt32) {
  FixedPointConverter converter("Variable32signed"); // default parameters are signed 32 bit
  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555));
  checkToCooked(converter, 0xAAAAAAAA, (int)0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (int)0x55555555);
  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_INT64(0xAAAAAAAA));
  checkToCooked(converter, 0x55555555, (uint64_t)0x55555555);

  checkToCookedOverflowNeg<unsigned int>(converter, 0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (unsigned int)0x55555555);
  checkToCookedOverflowNeg<short>(converter, 0xAAAAAAAA);
  checkToCookedOverflowPos<short>(converter, 0x55555555);
  checkToCookedOverflowNeg<unsigned short>(converter, 0xAAAAAAAA);
  checkToCookedOverflowPos<unsigned short>(converter, 0x55555555);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, -1);
  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, -3);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, -6);

  checkToRaw(converter, (int)0x55555555, 0x55555555);
  checkToRaw(converter, (int)0xAAAAAAAA, 0xAAAAAAAA);
  checkToRaw(converter, (unsigned int)0x55555555, 0x55555555);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x7FFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x5555);
  checkToRaw(converter, (short)0xAAAA, 0xFFFFAAAA);
  checkToRaw(converter, (unsigned short)0x5555, 0x5555);
  checkToRaw(converter, (unsigned short)0xAAAA, 0xAAAA);
  checkToRaw(converter, (int64_t)0x5555, 0x5555);
  checkToRaw(converter, (int64_t)0xFFFFFFFFFFFFAAAA, 0xFFFFAAAA);
  checkToRaw(converter, (int64_t)0xFFFFFFFAAAAAAAAA,
      0x80000000); // Smallest signed representation possible
  checkToRaw(converter, (int64_t)0xFFFFFFFFF, 0x7FFFFFFF);
  checkToRaw(converter, (uint64_t)0xFFFFFFFFF,
      0x7FFFFFFF); // max signed representation possible

  checkToCooked(converter, 0x55555555, std::string("1431655765"));
  checkToRaw(converter, std::string("1431655765"), 0x55555555);
}

BOOST_AUTO_TEST_CASE(testUInt32) {
  FixedPointConverter converter("Variable32unsigned", 32, 0,
      false); // 32 bits, 0 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555));
  checkToCookedOverflowPos<int>(converter, 0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (int)0x55555555);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (unsigned int)0x55555555);
  checkToCookedOverflowPos<short>(converter, 0xAAAAAAAA);
  checkToCookedOverflowPos<unsigned short>(converter, 0x55555555);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (uint64_t)0x55555555);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0);
  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x55555555, 0x55555555);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0x55555555);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0xAAAAAAAA);
  checkToRaw(converter, (short)0x5555, 0x5555);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x5555);
  checkToRaw(converter, (unsigned short)0xAAAA, 0xAAAA);
  checkToRaw(converter, (int64_t)0x5555, 0x5555);
  checkToRaw(converter, SIGNED_HEX_TO_INT64(0xAAAAAAAA),
      0x0); // Lowest range of 32 bit wide unsigned register
  checkToRaw(converter, (int64_t)0x100000000, 0xFFFFFFFF);
  checkToRaw(converter, (uint64_t)0x100000000,
      0xFFFFFFFF); // max signed representation possible

  checkToCooked(converter, 0x55555555, std::string("1431655765"));
  checkToRaw(converter, std::string("1431655765"), 0x55555555);

  checkToCooked(converter, 0xAAAAAAAA, std::string("2863311530"));
  checkToRaw(converter, std::string("2863311530"), 0xAAAAAAAA);
}

BOOST_AUTO_TEST_CASE(testInt16) {
  FixedPointConverter converter("Variable16signed",
      16); // 16 bits, 0 fractional bits, signed

  checkToCooked(converter, 0xAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFFAAAA));
  checkToCooked(converter, 0x5555, HEX_TO_DOUBLE(0x5555));
  checkToCooked(converter, 0xAAAA, (int)0xFFFFAAAA);
  checkToCooked(converter, 0x5555, (int)0x5555);
  checkToCookedOverflowNeg<unsigned int>(converter, 0xAAAA);
  checkToCooked(converter, 0x5555, (unsigned int)0x5555);
  checkToCooked(converter, 0xAAAA, (short)0xAAAA);
  checkToCooked(converter, 0x5555, (short)0x5555);
  checkToCookedOverflowNeg<unsigned short>(converter, 0xAAAA);
  checkToCooked(converter, 0x5555, (unsigned short)0x5555);
  checkToCooked(converter, 0x5555, (int64_t)0x5555);
  checkToCooked(converter, 0xAAAA, static_cast<int64_t>(static_cast<int16_t>(0xAAAA)));

  checkToCooked(converter, 0x5555, (uint64_t)0x5555);
  checkToCookedOverflowNeg<uint64_t>(converter, 0xAAAA);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0xFFFF);
  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0xFFFD);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0xFFFA);

  checkToRaw(converter, (int)0x55555555, 0x7FFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x8000);
  checkToRaw(converter, (unsigned int)0x55555555, 0x7FFF);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x7FFF);
  checkToRaw(converter, (short)0x5555, 0x5555);
  checkToRaw(converter, (short)0xAAAA, 0xAAAA);
  checkToRaw(converter, (unsigned short)0x5555, 0x5555);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x7FFF);
  checkToRaw(converter, (int64_t)0x5555, 0x5555);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int16_t>(0xAAAA)), 0xAAAA);
  checkToRaw(converter, (int64_t)0x555555, 0x7FFF);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xAAAAAAAA)), 0x8000);
  checkToRaw(converter, (uint64_t)0x5555, 0x5555);
  checkToRaw(converter, (uint64_t)0x0, 0x0);
  checkToRaw(converter, (uint64_t)0xF555, 0x7FFF);
}

BOOST_AUTO_TEST_CASE(testUInt16) {
  FixedPointConverter converter("Variable16unsigned", 16, 0,
      false); // 16 bits, 0 fractional bits, not signed

  checkToCooked(converter, 0xAAAA, HEX_TO_DOUBLE(0xAAAA));
  checkToCooked(converter, 0x5555, HEX_TO_DOUBLE(0x5555));
  checkToCooked(converter, 0xAAAA, (int)0xAAAA);
  checkToCooked(converter, 0x5555, (int)0x5555);
  checkToCooked(converter, 0xAAAA, (unsigned int)0xAAAA);
  checkToCooked(converter, 0x5555, (unsigned int)0x5555);
  checkToCookedOverflowPos<short>(converter, 0xAAAA);
  checkToCooked(converter, 0x5555, (short)0x5555);
  checkToCooked(converter, 0xAAAA, (unsigned short)0xAAAA);
  checkToCooked(converter, 0x5555, (unsigned short)0x5555);
  checkToCooked(converter, 0x5555, (int64_t)0x5555);
  checkToCooked(converter, 0xAAAA, (int64_t)0xAAAA);
  checkToCooked(converter, 0x5555, (uint64_t)0x5555);
  checkToCooked(converter, 0xAAAA, (uint64_t)0xAAAA);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0);
  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x55555555, 0xFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0xFFFF);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0xFFFF);
  checkToRaw(converter, (short)0x5555, 0x5555);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x5555);
  checkToRaw(converter, (unsigned short)0xAAAA, 0xAAAA);
  checkToRaw(converter, (int64_t)0x5555, 0x5555);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int16_t>(0xAAAA)), 0);
  checkToRaw(converter, (int64_t)0x555555, 0xFFFF);
  checkToRaw(converter, (uint64_t)0x5555, 0x5555);
  checkToRaw(converter, (uint64_t)0x0, 0x0);
  checkToRaw(converter, (uint64_t)0xFF555, 0xFFFF);
}

BOOST_AUTO_TEST_CASE(testInt8) {
  FixedPointConverter converter("Variable8signed",
      8); // 8 bits, 0 fractional bits, signed

  checkToCooked(converter, 0xAA, SIGNED_HEX_TO_DOUBLE(0xFFFFFFAA));
  checkToCooked(converter, 0x55, HEX_TO_DOUBLE(0x55));
  checkToCooked(converter, 0xAA, (int)0xFFFFFFAA);
  checkToCooked(converter, 0x55, (int)0x55);
  checkToCookedOverflowNeg<unsigned int>(converter, 0xAA);
  checkToCooked(converter, 0x55, (unsigned int)0x55);
  checkToCooked(converter, 0xAA, (short)0xFFAA);
  checkToCooked(converter, 0x55, (short)0x55);
  checkToCookedOverflowNeg<unsigned short>(converter, 0xAA);
  checkToCooked(converter, 0x55, (unsigned short)0x55);
  checkToCooked(converter, 0x55, (int64_t)0x55);
  checkToCooked(converter, 0xAA, static_cast<int64_t>(static_cast<int8_t>(0xAA)));
  checkToCooked(converter, 0x55, (uint64_t)0x55);
  checkToCookedOverflowNeg<uint64_t>(converter, 0xAA);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0xFF);
  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0xFD);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0xFA);

  checkToRaw(converter, (int)0x55555555, 0x7F);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x80);
  checkToRaw(converter, (unsigned int)0x55555555, 0x7F);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x7F);
  checkToRaw(converter, (short)0x5555, 0x7F);
  checkToRaw(converter, (short)0xAAAA, 0x80);
  checkToRaw(converter, (unsigned short)0x5555, 0x7F);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x7F);

  checkToRaw(converter, (int64_t)0x55, 0x55);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int8_t>(0xAA)), 0xAA);
  checkToRaw(converter, (int64_t)0x5555, 0x7F);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xAAAAAAAA)), 0x80);
  checkToRaw(converter, (uint64_t)0x55, 0x55);
  checkToRaw(converter, (uint64_t)0xF5, 0x7F);
}

BOOST_AUTO_TEST_CASE(testUInt8) {
  FixedPointConverter converter("Variable8unsigned", 8, 0,
      false); // 8 bits, 0 fractional bits, not signed

  checkToCooked(converter, 0xAA, HEX_TO_DOUBLE(0xAA));
  checkToCooked(converter, 0x55, HEX_TO_DOUBLE(0x55));
  checkToCooked(converter, 0xAA, (int)0xAA);
  checkToCooked(converter, 0x55, (int)0x55);
  checkToCooked(converter, 0xAA, (unsigned int)0xAA);
  checkToCooked(converter, 0x55, (unsigned int)0x55);
  checkToCooked(converter, 0xAA, (short)0xAA);
  checkToCooked(converter, 0x55, (short)0x55);
  checkToCooked(converter, 0xAA, (unsigned short)0xAA);
  checkToCooked(converter, 0x55, (unsigned short)0x55);
  checkToCooked(converter, 0x55, (int64_t)0x55);
  checkToCooked(converter, 0xAA, (int64_t)0xAA);
  checkToCooked(converter, 0x55, (uint64_t)0x55);
  checkToCooked(converter, 0xAA, (uint64_t)0xAA);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0);
  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x55555555, 0xFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0xFF);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0xFF);
  checkToRaw(converter, (short)0x5555, 0xFF);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0xFF);
  checkToRaw(converter, (unsigned short)0xAAAA, 0xFF);
  checkToRaw(converter, (int64_t)0x55, 0x55);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int16_t>(0xAAAA)), 0);
  checkToRaw(converter, (int64_t)0x555555, 0xFF);
  checkToRaw(converter, (uint64_t)0x55, 0x55);
  checkToRaw(converter, (uint64_t)0x0, 0x0);
  checkToRaw(converter, (uint64_t)0xFF555, 0xFF);
}

BOOST_AUTO_TEST_CASE(testInt32_fractionMinus12) {
  FixedPointConverter converter("Variable32minus12signed", 32,
      -12); // 32 bits, -12 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, 12)); // Basically a left shift 12
                                                                                       // places
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * pow(2, 12));
  checkToCookedOverflowPos<int>(converter, 0x000AAAAA);
  checkToCooked(converter, 0xFFFAAAAA, (int)0xAAAAA000);
  checkToCooked(converter, 0x00055555, (int)0x55555000);
  checkToCookedOverflowNeg<unsigned int>(converter, 0xFFFAAAAA);
  checkToCooked(converter, 0x00055555, (unsigned int)0x55555000);
  checkToCooked(converter, 0x000AAAAA, (unsigned int)0xAAAAA000);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0xFFFFFAAAAAAAA000);
  checkToCooked(converter, 0x55555555, (int64_t)0x55555555000);
  checkToCooked(converter, 0x55555555, (uint64_t)0x55555555000);
  checkToCookedOverflowNeg<uint64_t>(converter, 0xAAAAAAAA);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);
  checkToRaw(converter, 3.25, 0);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 0);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x55555555, 0x00055555);
  checkToRaw(converter, (int)0xAAAAAAAA, 0xFFFAAAAB);
  checkToRaw(converter, (unsigned int)0x55555555, 0x00055555);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x000AAAAB);
  checkToRaw(converter, (short)0x5555, 0x00000005);
  checkToRaw(converter, (short)0xAAAA, 0xFFFFFFFB);
  checkToRaw(converter, (unsigned short)0x5555, 0x00000005);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x0000000B);
  checkToRaw(converter, (int64_t)0x55555555, 0x00055555);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xAAAAAAAA)), 0XFFFAAAAB);
  checkToRaw(converter, (int64_t)0x5555555555555, 0x7fffffff); // full range
  checkToRaw(converter, (int64_t)0xFFFFA55555555555, 0x80000000);
  checkToRaw(converter, (uint64_t)0x55555, 0x00000055);
  checkToRaw(converter, (uint64_t)0x5555555555555, 0X7FFFFFFF);
}

BOOST_AUTO_TEST_CASE(testUInt32_fractionMinus12) {
  FixedPointConverter converter("Variable32minus12unsigned", 32, -12,
      false); // 32 bits, -12 fractional bits, not
              // signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, 12));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * pow(2, 12));
  checkToCookedOverflowPos<int>(converter, 0x000AAAAA);
  checkToCooked(converter, 0x00055555, (int)0x55555000);
  checkToCooked(converter, 0x00055555, (unsigned int)0x55555000);
  checkToCooked(converter, 0x000AAAAA, (unsigned int)0xAAAAA000);
  checkToCookedOverflowPos<unsigned short>(converter, 0x000AAAAA);
  checkToCooked(converter, 0x00055555, (int64_t)0x55555000);
  checkToCooked(converter, 0x000AAAAA, (int64_t)0xAAAAA000);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0xAAAAAAAA000);
  checkToCooked(converter, 0x00055555, (uint64_t)0x55555000);
  checkToCooked(converter, 0xAAAAAAAA, (uint64_t)0xAAAAAAAA000);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);
  checkToRaw(converter, 3.25, 0);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 0);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x55555555, 0x00055555);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0x00055555);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x000AAAAB);
  checkToRaw(converter, (short)0x5555, 0x00000005);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x00000005);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x0000000B);
  checkToRaw(converter, (int64_t)0x55555555, 0x00055555);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xAAAAAAAA)), 0x0);
  checkToRaw(converter, (int64_t)0x5555555555555, 0xFFFFFFFF); // full range
  checkToRaw(converter, (uint64_t)0x55555, 0x00000055);
  checkToRaw(converter, (uint64_t)0x5555555555555, 0XFFFFFFFF);
}

BOOST_AUTO_TEST_CASE(testInt32_fractionMinus1) {
  FixedPointConverter converter("Variable32minus1signed", 32,
      -1); // 32 bits, -1 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * 2);
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * 2);
  checkToCookedOverflowNeg<int>(converter, 0xAAAAAAAA);
  checkToCookedOverflowPos<int>(converter, 0x55555555);
  checkToCooked(converter, 0x22222202, (int)0x44444404);
  checkToCookedOverflowNeg<unsigned int>(converter, 0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (unsigned int)0xAAAAAAAA);
  checkToCooked(converter, 0x22222202, (unsigned int)0x44444404);
  checkToCooked(converter, 0x7FFFFFFF, (unsigned int)0xFFFFFFFE);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0xFFFFFFFF55555554);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);

  // bit pattern of 3 is 11, where the last digit is rounded up, and afterwards
  // one bit is shifted. So the actual value is 4
  checkToRaw(converter, 3.25, 0x2);
  checkToRaw(converter, -3.25, 0xFFFFFFFE); // (-2)
  checkToRaw(converter, 5.75, 0x3);
  checkToRaw(converter, -5.75, 0xFFFFFFFD); // (-3)

  checkToRaw(converter, (int)0x55555554, 0x2AAAAAAA);
  checkToRaw(converter, (int)0x55555555, 0x2AAAAAAB);
  checkToRaw(converter, (int)0x55555556, 0x2AAAAAAB);
  checkToRaw(converter, (int)0xAAAAAAAA, 0xD5555555);
  checkToRaw(converter, (unsigned int)0x55555555, 0x2AAAAAAB);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x55555555);
  checkToRaw(converter, (short)0x5555, 0x00002AAB);
  checkToRaw(converter, (short)0xAAAA, 0xFFFFD555);
  checkToRaw(converter, (unsigned short)0x5555, 0x00002AAB);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00005555);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xAAAAAAAA)), 0xD5555555);
  checkToRaw(converter, static_cast<uint64_t>(0xAAAAAAAA), 0x55555555);
}

BOOST_AUTO_TEST_CASE(testUInt32_fractionMinus1) {
  FixedPointConverter converter("Variable32minus1unsigned", 32, -1,
      false); // 32 bits, -1 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * 2);
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * 2);
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * 2);
  checkToCooked(converter, 0x22222202, (int)0x44444404);
  checkToCooked(converter, 0x55555555, (unsigned int)0xAAAAAAAA);
  checkToCooked(converter, 0x22222202, (unsigned int)0x44444404);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);

  // bit pattern of 3 is 11, where the last digit is rounded up, and afterwards
  // one bit is shifted. So the actual value is 4
  checkToRaw(converter, 3.25, 0x2);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 0x3);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x55555555, 0x2AAAAAAB);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0x2AAAAAAB);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x55555555);
  checkToRaw(converter, (short)0x5555, 0x00002AAB);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x00002AAB);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00005555);
}

BOOST_AUTO_TEST_CASE(testInt16_fractionMinus1) {
  FixedPointConverter converter("Variable16minus1signed", 16, -1); // 16 bits, -1 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFFAAAA) * 2);
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x5555) * 2);
  checkToCookedOverflowNeg<int16_t>(converter, 0xAAAAAAAA);
  checkToCookedOverflowPos<int16_t>(converter, 0x55555555);
  checkToCooked(converter, 0x22222202, (int)0x4404);
  checkToCookedOverflowNeg<unsigned int>(converter, 0xAAAA);
  checkToCooked(converter, 0x55555555, (unsigned int)0xAAAA);
  checkToCooked(converter, 0x22222202, (unsigned int)0x4404);
  checkToCooked(converter, 0x00007FFF, (unsigned int)0xFFFE);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0xFFFFFFFFFFFF5554);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);

  // bit pattern of 3 is 11, where the last digit is rounded up, and afterwards
  // one bit is shifted. So the actual value is 4
  checkToRaw(converter, 3.25, 0x2);
  checkToRaw(converter, -3.25, 0xFFFE); // (-2)
  checkToRaw(converter, 5.75, 0x3);
  checkToRaw(converter, -5.75, 0xFFFD); // (-3)

  checkToRaw(converter, (int)0x5554, 0x2AAA);
  checkToRaw(converter, (int)0x5555, 0x2AAB);
  checkToRaw(converter, (int)0x5556, 0x2AAB);
  checkToRaw(converter, (int)0xFFFFAAAA, 0xD555);
  checkToRaw(converter, (unsigned int)0x5555, 0x2AAB);
  checkToRaw(converter, (unsigned int)0xAAAA, 0x5555);
  checkToRaw(converter, (short)0x5555, 0x00002AAB);
  checkToRaw(converter, (short)0xAAAA, 0xD555);
  checkToRaw(converter, (unsigned short)0x5555, 0x00002AAB);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00005555);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xFFFFAAAA)), 0xD555);
  checkToRaw(converter, static_cast<uint64_t>(0xAAAA), 0x5555);
}

BOOST_AUTO_TEST_CASE(testUInt16_fractionMinus1) {
  FixedPointConverter converter("Variable16minus1unsigned", 16, -1, false); // 16 bits, -1 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAA) * 2);
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x5555) * 2);
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x5555) * 2);
  checkToCooked(converter, 0x22222202, (int)0x4404);
  checkToCooked(converter, 0x55555555, (unsigned int)0xAAAA);
  checkToCooked(converter, 0x22222202, (unsigned int)0x4404);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);

  // bit pattern of 3 is 11, where the last digit is rounded up, and afterwards
  // one bit is shifted. So the actual value is 4
  checkToRaw(converter, 3.25, 0x2);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 0x3);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0x5555, 0x2AAB);
  checkToRaw(converter, (int)0xFFFFAAAA, 0);
  checkToRaw(converter, (unsigned int)0x5555, 0x2AAB);
  checkToRaw(converter, (unsigned int)0xAAAA, 0x5555);
  checkToRaw(converter, (short)0x5555, 0x00002AAB);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x00002AAB);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00005555);
}

BOOST_AUTO_TEST_CASE(testInt32_fraction1) {
  FixedPointConverter converter("Variable32plus1signed", 32,
      1); // 32 bits, 1 fractional bit, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * 0.5);
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * 0.5);
  checkToCooked(converter, 0xAAAAAAA9, (int)0xD5555554);
  checkToCooked(converter, 0xAAAAAAAA, (int)0xD5555555);
  checkToCooked(converter, 0xAAAAAAAB, (int)0xD5555555);
  checkToCooked(converter, 0xFFFFFE11, (int)0xFFFFFF08);
  checkToCooked(converter, 0x55555554, (int)0x2AAAAAAA);
  checkToCooked(converter, 0x55555555, (int)0x2AAAAAAB);
  checkToCooked(converter, 0x55555556, (int)0x2AAAAAAB);
  checkToCooked(converter, 0x22222202, (int)0x11111101);
  checkToCooked(converter, 0x55555555, (unsigned int)0x2AAAAAAB);
  checkToCooked(converter, 0x22222202, (unsigned int)0x11111101);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0xFFFFFFFFD5555555);
  checkToCooked(converter, 0x55555555, (int64_t)0x2AAAAAAB);

  checkToRaw(converter, 0.25, 0x1);
  checkToRaw(converter, -0.25, 0xFFFFFFFF);
  checkToRaw(converter, 0.75, 0x2);
  checkToRaw(converter, -0.75, 0xFFFFFFFE);

  checkToRaw(converter, 3.25, 0x7);
  checkToRaw(converter, -3.25, 0xFFFFFFF9); // (-7)
  checkToRaw(converter, 5.75, 0xC);
  checkToRaw(converter, -5.75, 0xFFFFFFF4); // (-12)

  checkToRaw(converter, (int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x80000000);
  checkToRaw(converter, (int)0x22222202, 0x44444404);
  checkToRaw(converter, (int)0xE2222202, 0xC4444404);
  checkToRaw(converter, (unsigned int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x7FFFFFFF);
  checkToRaw(converter, (unsigned int)0x22222202, 0x44444404);
  checkToRaw(converter, (unsigned int)0xE2222202, 0x7FFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x0000AAAA);
  checkToRaw(converter, (short)0xAAAA, 0xFFFF5554);
  checkToRaw(converter, (unsigned short)0x5555, 0x0000AAAA);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00015554);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xFAAAAAAA)), 0XF5555554);
  checkToRaw(converter, static_cast<uint64_t>(0xAAAAAAA), 0X15555554);
}

BOOST_AUTO_TEST_CASE(testUInt32_fraction1) {
  FixedPointConverter converter("Variable32plus1unsigned", 32, 1,
      false); // 32 bits, 1 fractional bit, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * 0.5);
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * 0.5);
  checkToCooked(converter, 0xAAAAAAAA, (int)0x55555555);
  checkToCooked(converter, 0x55555555, (int)0x2AAAAAAB);
  checkToCooked(converter, 0x22222202, (int)0x11111101);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)0x55555555);
  checkToCooked(converter, 0x55555555, (unsigned int)0x2AAAAAAB);
  checkToCooked(converter, 0x22222202, (unsigned int)0x11111101);
  checkToCooked(converter, 0xAAAAAAAA, (int64_t)0x55555555);
  checkToCooked(converter, 0x55555555, (int64_t)0x2AAAAAAB);

  checkToRaw(converter, 0.25, 0x1);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x2);
  checkToRaw(converter, -0.75, 0x0);

  checkToRaw(converter, 3.25, 0x7);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0xC);
  checkToRaw(converter, -5.75, 0x0);

  checkToRaw(converter, (int)0x55555555, 0xAAAAAAAA);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (int)0x22222202, 0x44444404);
  checkToRaw(converter, (int)0xE2222202, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0xAAAAAAAA);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0xFFFFFFFF);
  checkToRaw(converter, (unsigned int)0x22222202, 0x44444404);
  checkToRaw(converter, (unsigned int)0xE2222202, 0xFFFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x0000AAAA);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x0000AAAA);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00015554);
  checkToRaw(converter, static_cast<int64_t>(static_cast<int32_t>(0xFAAAAAAA)), 0x0);
  checkToRaw(converter, static_cast<uint64_t>(0xFAAAAAAA), 0XFFFFFFFF);
}

BOOST_AUTO_TEST_CASE(testInt32_fraction7) {
  FixedPointConverter converter("Variable32plus7signed", 32,
      7); // 32 bits, 7 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -7));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * pow(2, -7));
  checkToCooked(converter, 0xAAAAAA20, (int)0xFF555554);
  checkToCooked(converter, 0xAAAAAA60, (int)0xFF555555);
  checkToCooked(converter, 0xAAAAAA80, (int)0xFF555555);
  checkToCooked(converter, 0xAAAAAAAA, (int)0xFF555555);
  checkToCooked(converter, 0xAAAAAAC0, (int)0xFF555555);
  checkToCooked(converter, 0xAAAAAAD0, (int)0xFF555556);
  checkToCooked(converter, 0xAAAAAAFF, (int)0xFF555556);
  checkToCooked(converter, 0x5555553F, (int)0x00AAAAAA);
  checkToCooked(converter, 0x55555540, (int)0x00AAAAAB);
  checkToCooked(converter, 0x555555BF, (int)0x00AAAAAB);
  checkToCooked(converter, 0x555555C0, (int)0x00AAAAAC);
  checkToCooked(converter, 0x22220222, (int)0x00444404);
  checkToCooked(converter, 0x55555555, (unsigned int)0x00AAAAAB);
  checkToCooked(converter, 0x22220222, (unsigned int)0x00444404);

  checkToRaw(converter, 0.25, 0x20);
  checkToRaw(converter, -0.25, 0xFFFFFFE0);
  checkToRaw(converter, 0.75, 0x60);
  checkToRaw(converter, -0.75, 0xFFFFFFA0);

  checkToRaw(converter, 3.25, 0x1A0);
  checkToRaw(converter, -3.25, 0xFFFFFE60);
  checkToRaw(converter, 5.75, 0x2E0);
  checkToRaw(converter, -5.75, 0xFFFFFD20);

  checkToRaw(converter, (int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x80000000);
  checkToRaw(converter, (int)0x00888808, 0x44440400);
  checkToRaw(converter, (int)0xFF888808, 0xC4440400);
  checkToRaw(converter, (unsigned int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0x7FFFFFFF);
  checkToRaw(converter, (unsigned int)0x00888808, 0x44440400);
  checkToRaw(converter, (unsigned int)0xFF888808, 0x7FFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x002AAA80);
  checkToRaw(converter, (short)0xAAAA, 0xFFD55500);
  checkToRaw(converter, (unsigned short)0x5555, 0x002AAA80);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00555500);

  checkToCooked(converter, 0x20, std::string("0.250000"));
  checkToRaw(converter, std::string("0.25"), 0x20);
}

BOOST_AUTO_TEST_CASE(testUInt32_fraction7) {
  FixedPointConverter converter("Variable32plus7unsigned", 32, 7,
      false); // 32 bits, -7 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -7));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * pow(2, -7));
  checkToCooked(converter, 0xAAAAAAAA, (int)0x01555555);
  checkToCooked(converter, 0x55555555, (int)0x00AAAAAB);
  checkToCooked(converter, 0x22220222, (int)0x00444404);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)0x01555555);
  checkToCooked(converter, 0x55555555, (unsigned int)0x00AAAAAB);
  checkToCooked(converter, 0x22220222, (unsigned int)0x00444404);

  checkToRaw(converter, 0.25, 0x20);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x60);
  checkToRaw(converter, -0.75, 0x0);

  checkToRaw(converter, 3.25, 0x1A0);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0x2E0);
  checkToRaw(converter, -5.75, 0x0);

  checkToRaw(converter, (int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (int)0x00888808, 0x44440400);
  checkToRaw(converter, (int)0xFF888808, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (unsigned int)0xAAAAAAAA, 0xFFFFFFFF);
  checkToRaw(converter, (unsigned int)0x00888808, 0x44440400);
  checkToRaw(converter, (unsigned int)0xFF888808, 0xFFFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x002AAA80);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0x002AAA80);
  checkToRaw(converter, (unsigned short)0xAAAA, 0x00555500);
}

BOOST_AUTO_TEST_CASE(testInt32_fraction31) {
  FixedPointConverter converter("Variable32plus31signed", 32,
      31); // 32 bits, 31 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -31));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * pow(2, -31));
  checkToCooked(converter, 0xAAAAAAAA, (int)-1);
  checkToCooked(converter, 0x55555555, (int)1);
  checkToCooked(converter, 0x22220222, (int)0);
  checkToCookedOverflowNeg<unsigned int>(converter, 0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (unsigned int)1);
  checkToCooked(converter, 0x22220222, (unsigned int)0);
  checkToCooked(converter, 0xAAAAAAAA, (short)-1);
  checkToCooked(converter, 0x55555555, (short)1);
  checkToCookedOverflowNeg<unsigned short>(converter, 0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (unsigned short)1);

  checkToRaw(converter, 0.25, 0x20000000);
  checkToRaw(converter, -0.25, 0xE0000000);
  checkToRaw(converter, 0.75, 0x60000000);
  checkToRaw(converter, -0.75, 0xA0000000);

  // these values are out of range
  checkToRaw(converter, 3.25, 0x7FFFFFFF);
  checkToRaw(converter, -3.25, 0x80000000);
  checkToRaw(converter, 5.75, 0x7FFFFFFF);
  checkToRaw(converter, -5.75, 0x80000000);

  checkToCooked(converter, 0xA0000000, -0.75);
  checkToCooked(converter, 0x60000000, 0.75);
  checkToCooked(converter, 0xE0000000, -0.25);
  checkToCooked(converter, 0x20000000, 0.25);

  checkToRaw(converter, (int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x80000000);
  checkToRaw(converter, (int)0, 0);
  checkToRaw(converter, (int)-1, 0x80000000);
  checkToRaw(converter, (unsigned int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x7FFFFFFF);
  checkToRaw(converter, (short)0xAAAA, 0x80000000);
  checkToRaw(converter, (short)-1, 0x80000000);
  checkToRaw(converter, (unsigned short)0x5555, 0x7FFFFFFF);
}

BOOST_AUTO_TEST_CASE(testUInt32_fraction31) {
  FixedPointConverter converter("Variable32plus31unsigned", 32, 31,
      false); // 32 bits, 31 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -31));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * pow(2, -31));
  checkToCooked(converter, 0xAAAAAAAA, (int)1);
  checkToCooked(converter, 0x55555555, (int)1);
  checkToCooked(converter, 0x22220222, (int)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)1);
  checkToCooked(converter, 0x55555555, (unsigned int)1);
  checkToCooked(converter, 0x22220222, (unsigned int)0);
  checkToCooked(converter, 0xAAAAAAAA, (short)1);
  checkToCooked(converter, 0x55555555, (short)1);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned short)1);
  checkToCooked(converter, 0x55555555, (unsigned short)1);

  checkToRaw(converter, 0.25, 0x20000000);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x60000000);
  checkToRaw(converter, -0.75, 0x0);

  // these values are out of range
  checkToRaw(converter, 3.25, 0xFFFFFFFF);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0xFFFFFFFF);
  checkToRaw(converter, -5.75, 0x0);

  checkToCooked(converter, 0xA0000000, 1.25);
  checkToCooked(converter, 0x60000000, 0.75);
  checkToCooked(converter, 0xE0000000, 1.75);
  checkToCooked(converter, 0x20000000, 0.25);

  checkToRaw(converter, (int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (int)1, 0x80000000);
  checkToRaw(converter, (unsigned int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (unsigned int)1, 0x80000000);
  checkToRaw(converter, (short)0x5555, 0xFFFFFFFF);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (short)1, 0x80000000);
  checkToRaw(converter, (unsigned short)0x5555, 0xFFFFFFFF);
  checkToRaw(converter, (unsigned short)1, 0x80000000);
}

BOOST_AUTO_TEST_CASE(testInt32_fraction32) {
  FixedPointConverter converter("Variable32plus32signed", 32,
      32); // 32 bits, 32 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -32));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * pow(2, -32));
  checkToCooked(converter, 0xAAAAAAAA, (int)0);
  checkToCooked(converter, 0x55555555, (int)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)0);
  checkToCooked(converter, 0x55555555, (unsigned int)0);
  checkToCooked(converter, 0xAAAAAAAA, (short)0);
  checkToCooked(converter, 0x55555555, (short)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned short)0);
  checkToCooked(converter, 0x55555555, (unsigned short)0);

  checkToRaw(converter, 0.25, 0x40000000);
  checkToRaw(converter, -0.25, 0xC0000000);

  // these values are out of range
  checkToRaw(converter, 0.75, 0x7FFFFFFF);
  checkToRaw(converter, -0.75, 0x80000000);
  checkToRaw(converter, 3.25, 0x7FFFFFFF);
  checkToRaw(converter, -3.25, 0x80000000);
  checkToRaw(converter, 5.75, 0x7FFFFFFF);
  checkToRaw(converter, -5.75, 0x80000000);

  checkToCooked(converter, 0x40000000, 0.25);
  checkToCooked(converter, 0xC0000000, -0.25);

  checkToRaw(converter, (int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x80000000);
  checkToRaw(converter, (int)0, 0);
  checkToRaw(converter, (int)1, 0x7FFFFFFF);
  checkToRaw(converter, (int)-1, 0x80000000);
  // checkToRaw( converter, (unsigned int)0x55555555, 0x7FFFFFFF );
  checkToRaw(converter, (short)0x5555, 0x7FFFFFFF);
  checkToRaw(converter, (short)0xAAAA, 0x80000000);
  checkToRaw(converter, (short)-1, 0x80000000);
  // checkToRaw( converter, (unsigned short)0x5555, 0x7FFFFFFF );
}

BOOST_AUTO_TEST_CASE(testUInt32_fraction32) {
  FixedPointConverter converter("Variable32plus32unsigned", 32, 32,
      false); // 32 bits, 32 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -32));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * pow(2, -32));
  checkToCooked(converter, 0xAAAAAAAA, (int)1);
  checkToCooked(converter, 0x55555555, (int)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)1);
  checkToCooked(converter, 0x55555555, (unsigned int)0);
  checkToCooked(converter, 0xAAAAAAAA, (short)1);
  checkToCooked(converter, 0x55555555, (short)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned short)1);
  checkToCooked(converter, 0x55555555, (unsigned short)0);

  checkToRaw(converter, 0.25, 0x40000000);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0xC0000000);
  checkToRaw(converter, -0.75, 0x0);

  // these values are out of range
  checkToRaw(converter, 3.25, 0xFFFFFFFF);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0xFFFFFFFF);
  checkToRaw(converter, -5.75, 0x0);

  checkToCooked(converter, 0x40000000, 0.25);
  checkToCooked(converter, 0xC0000000, 0.75);

  checkToRaw(converter, (int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (int)0, 0);
  checkToRaw(converter, (int)-1, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (short)0x5555, 0xFFFFFFFF);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (short)-1, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0xFFFFFFFF);
}

BOOST_AUTO_TEST_CASE(testInt32_fraction43) {
  FixedPointConverter converter("Variable32plus43signed", 32,
      43); // 32 bits, 43 fractional bits, signed

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -43));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x55555555) * pow(2, -43));
  checkToCooked(converter, 0xAAAAAAAA, (int)0);
  checkToCooked(converter, 0x55555555, (int)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)0);
  checkToCooked(converter, 0x55555555, (unsigned int)0);
  checkToCooked(converter, 0xAAAAAAAA, (short)0);
  checkToCooked(converter, 0x55555555, (short)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned short)0);
  checkToCooked(converter, 0x55555555, (unsigned short)0);
  checkToCooked(converter, 0x555, (int64_t)0);
  checkToCooked(converter, 0x555, (uint64_t)0);

  // all out of range
  checkToRaw(converter, 0.25, 0x7FFFFFFF);
  checkToRaw(converter, -0.25, 0x80000000);
  checkToRaw(converter, 0.75, 0x7FFFFFFF);
  checkToRaw(converter, -0.75, 0x80000000);

  checkToRaw(converter, 3.25, 0x7FFFFFFF);
  checkToRaw(converter, -3.25, 0x80000000);
  checkToRaw(converter, 5.75, 0x7FFFFFFF);
  checkToRaw(converter, -5.75, 0x80000000);

  checkToRaw(converter, (int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0x80000000);
  checkToRaw(converter, (int)0, 0);
  checkToRaw(converter, (int)-1, 0x80000000);
  checkToRaw(converter, (unsigned int)0x55555555, 0x7FFFFFFF);
  checkToRaw(converter, (short)0x5555, 0x7FFFFFFF);
  checkToRaw(converter, (short)0xAAAA, 0x80000000);
  checkToRaw(converter, (short)-1, 0x80000000);
  checkToRaw(converter, (unsigned short)0x5555, 0x7FFFFFFF);
  checkToRaw(converter, (int64_t)0xFFFFFFFAAAAAAAAA, 0x80000000);
  checkToRaw(converter, (uint64_t)0xAAAAAAAAA, 0x7FFFFFFF);
}

BOOST_AUTO_TEST_CASE(testUInt32_fraction43) {
  FixedPointConverter converter("Variable32plus43unsigned", 32, 43,
      false); // 32 bits, 43 fractional bits, not signed

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0xAAAAAAAA) * pow(2, -43));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x55555555) * pow(2, -43));
  checkToCooked(converter, 0xAAAAAAAA, (int)0);
  checkToCooked(converter, 0x55555555, (int)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned int)0);
  checkToCooked(converter, 0x55555555, (unsigned int)0);
  checkToCooked(converter, 0xAAAAAAAA, (short)0);
  checkToCooked(converter, 0x55555555, (short)0);
  checkToCooked(converter, 0xAAAAAAAA, (unsigned short)0);
  checkToCooked(converter, 0x55555555, (unsigned short)0);

  // all out of range
  checkToRaw(converter, 0.25, 0xFFFFFFFF);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0xFFFFFFFF);
  checkToRaw(converter, -0.75, 0x0);

  checkToRaw(converter, 3.25, 0xFFFFFFFF);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0xFFFFFFFF);
  checkToRaw(converter, -5.75, 0x0);

  checkToRaw(converter, (int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (int)0xAAAAAAAA, 0);
  checkToRaw(converter, (int)0, 0);
  checkToRaw(converter, (int)-1, 0);
  checkToRaw(converter, (unsigned int)0x55555555, 0xFFFFFFFF);
  checkToRaw(converter, (short)0x5555, 0xFFFFFFFF);
  checkToRaw(converter, (short)0xAAAA, 0);
  checkToRaw(converter, (short)-1, 0);
  checkToRaw(converter, (unsigned short)0x5555, 0xFFFFFFFF);
}

BOOST_AUTO_TEST_CASE(testInt18_fractionMinus12) {
  FixedPointConverter converter("int18_fractionMinus12", 18,
      -12); // 10 bits, -12 fractional bits, signed

  checkToCooked(converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, 12));
  checkToCooked(converter, 0x15555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, 12));

  // the converter should ignore bits which are not in the spec
  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, 12));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, 12));

  checkToCooked(converter, 0x2AAAA, (int)0xEAAAA000);
  checkToCooked(converter, 0x15555, (int)0x15555000);
  checkToCooked(converter, 0x15555, (unsigned int)0x15555000);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);

  checkToRaw(converter, 3.25, 0);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 0);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0xEAAAA000, 0x2AAAA);
  checkToRaw(converter, (int)0x15555000, 0x15555);
  checkToRaw(converter, (unsigned int)0x15555000, 0x15555);
  checkToRaw(converter, (short)0xA000, 0x3FFFA);
  checkToRaw(converter, (short)0x5000, 0x00005);
  checkToRaw(converter, (unsigned short)0xA000, 0x0000A);
}

BOOST_AUTO_TEST_CASE(testUInt18_fractionMinus12) {
  FixedPointConverter converter("Variable18minus12unsigned", 18, -12,
      false); // 10 bits, -12 fractional bits, not
              // signed

  checkToCooked(converter, 0x2AAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, 12));
  checkToCooked(converter, 0x15555, HEX_TO_DOUBLE(0x15555) * pow(2, 12));

  // the converter should ignore bits which are not in the spec
  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, 12));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x15555) * pow(2, 12));

  checkToCooked(converter, 0x2AAAA, (int)0x2AAAA000);
  checkToCooked(converter, 0x15555, (int)0x15555000);
  checkToCooked(converter, 0x2AAAA, (unsigned int)0x2AAAA000);
  checkToCooked(converter, 0x15555, (unsigned int)0x15555000);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 0);
  checkToRaw(converter, -0.75, 0);

  checkToRaw(converter, 3.25, 0);
  checkToRaw(converter, -3.25, 0);
  checkToRaw(converter, 5.75, 0);
  checkToRaw(converter, -5.75, 0);

  checkToRaw(converter, (int)0xEAAAA000, 0);
  checkToRaw(converter, (int)0x15555000, 0x15555);
  checkToRaw(converter, (unsigned int)0x15555000, 0x15555);
  checkToRaw(converter, (short)0xA000, 0);
  checkToRaw(converter, (short)0x5000, 0x00005);
  checkToRaw(converter, (unsigned short)0xA000, 0x0000A);
}

BOOST_AUTO_TEST_CASE(testInt18_fraction0) {
  FixedPointConverter converter("Variable18minus12signed",
      18); // 18 bits, 0 fractional bits, signed

  checkToCooked(converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA));
  checkToCooked(converter, 0x15555, SIGNED_HEX_TO_DOUBLE(0x15555));

  // the converter should ignore bits which are not in the spec
  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x15555));

  checkToCooked(converter, 0x2AAAA, (int)0xFFFEAAAA);
  checkToCooked(converter, 0x15555, (int)0x15555);
  checkToCooked(converter, 0x15555, (unsigned int)0x15555);

  checkToCooked(converter, 0x2AAAA, (int64_t)0xFFFFFFFFFFFEAAAA);
  checkToCooked(converter, 0x15555, (int64_t)0x15555);
  checkToCooked(converter, 0x15555, (uint64_t)0x15555);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0x3FFFF);

  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0x3FFFD);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0x3FFFA);

  checkToRaw(converter, (int)0xFFFEAAAA, 0x2AAAA);
  checkToRaw(converter, (int)0x00015555, 0x15555);
  checkToRaw(converter, (unsigned int)0x00015555, 0x15555);
  checkToRaw(converter, (short)0xA000, 0x3A000);
  checkToRaw(converter, (short)0x5000, 0x05000);
  checkToRaw(converter, (unsigned short)0xA000, 0x0A000);

  checkToRaw(converter, (int64_t)0xFFFFFFFFFFFFA000, 0x3A000);
  checkToRaw(converter, (int64_t)0xA000, 0xA000);
  checkToRaw(converter, (uint64_t)0xA000, 0x0A000);
}

BOOST_AUTO_TEST_CASE(testUInt18_fraction0) {
  FixedPointConverter converter("Variable18unsigned", 18, 0,
      false); // 10 bits, -12 fractional bits, not signed

  checkToCooked(converter, 0x2AAAA, HEX_TO_DOUBLE(0x2AAAA));
  checkToCooked(converter, 0x15555, HEX_TO_DOUBLE(0x15555));

  // the converter should ignore bits which are not in the spec
  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0x2AAAA));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x15555));

  checkToCooked(converter, 0x2AAAA, (int)0x2AAAA);
  checkToCooked(converter, 0x15555, (int)0x15555);
  checkToCooked(converter, 0x2AAAA, (unsigned int)0x2AAAA);
  checkToCooked(converter, 0x15555, (unsigned int)0x15555);

  checkToRaw(converter, 0.25, 0);
  checkToRaw(converter, -0.25, 0);
  checkToRaw(converter, 0.75, 1);
  checkToRaw(converter, -0.75, 0x0);

  checkToRaw(converter, 3.25, 3);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 6);
  checkToRaw(converter, -5.75, 0x0);

  checkToRaw(converter, (int)0xFFFEAAAA, 0);
  checkToRaw(converter, (int)0x00015555, 0x15555);
  checkToRaw(converter, (unsigned int)0x00015555, 0x15555);
  checkToRaw(converter, (short)0xA000, 0);
  checkToRaw(converter, (short)0x5000, 0x05000);
  checkToRaw(converter, (unsigned short)0xA000, 0x0A000);
}

BOOST_AUTO_TEST_CASE(testInt18_fraction7) {
  FixedPointConverter converter("Variable18plus7signed", 18,
      7); // 18 bits, 7 fractional bits, signed

  checkToCooked(converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -7));
  checkToCooked(converter, 0x15555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -7));

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -7));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -7));

  checkToCooked(converter, 0x2AAAA, (int)0xFFFFFD55);
  checkToCooked(converter, 0x15555, (int)0x02AB);
  checkToCooked(converter, 0x15555, (unsigned int)0x2AB);

  checkToRaw(converter, 0.25, 0x20);
  checkToRaw(converter, -0.25, 0x3FFE0);
  checkToRaw(converter, 0.75, 0x60);
  checkToRaw(converter, -0.75, 0x3FFA0);

  checkToRaw(converter, 3.25, 0x1A0);
  checkToRaw(converter, -3.25, 0x3FE60);
  checkToRaw(converter, 5.75, 0x2E0);
  checkToRaw(converter, -5.75, 0x3FD20);

  checkToRaw(converter, (int)0xFFFFFD55, 0x2AA80);
  checkToRaw(converter, (int)0x02AA, 0x15500);
  checkToRaw(converter, (unsigned int)0x2AA, 0x15500);
  checkToRaw(converter, (short)0xFFAA, 0x3D500);
  checkToRaw(converter, (short)0x0055, 0x02A80);
  checkToRaw(converter, (unsigned short)0x0055, 0x02A80);
}

BOOST_AUTO_TEST_CASE(testUInt18_fraction7) {
  FixedPointConverter converter("Variable18plus7unsigned", 18, 7,
      false); // 10 bits, -12 fractional bits, not signed

  checkToCooked(converter, 0x2AAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -7));
  checkToCooked(converter, 0x15555, HEX_TO_DOUBLE(0x15555) * pow(2, -7));

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -7));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x15555) * pow(2, -7));

  checkToCooked(converter, 0x2AAAA, (int)0x0555);
  checkToCooked(converter, 0x15555, (int)0x02AB);
  checkToCooked(converter, 0x2AAAA, (unsigned int)0x0555);
  checkToCooked(converter, 0x15555, (unsigned int)0x02AB);
  checkToCooked(converter, 0x2AAAA, (short)0x0555);
  checkToCooked(converter, 0x15555, (short)0x02AB);
  checkToCooked(converter, 0x2AAAA, (unsigned short)0x0555);
  checkToCooked(converter, 0x15555, (unsigned short)0x02AB);

  checkToRaw(converter, 0.25, 0x20);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x60);
  checkToRaw(converter, -0.75, 0x0);

  checkToRaw(converter, 3.25, 0x1A0);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0x2E0);
  checkToRaw(converter, -5.75, 0x0);

  checkToRaw(converter, (int)0x0555, 0x2AA80);
  checkToRaw(converter, (int)0x02AA, 0x15500);
  checkToRaw(converter, (unsigned int)0x02AA, 0x15500);
  checkToRaw(converter, (short)0xFFAA, 0);
  checkToRaw(converter, (short)0x0055, 0x02A80);
  checkToRaw(converter, (unsigned short)0x0055, 0x02A80);
}

BOOST_AUTO_TEST_CASE(testInt18_fraction17) {
  FixedPointConverter converter("Variable18plus17signed", 18,
      17); // 18 bits, 17 fractional bits, signed

  checkToCooked(converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -17));
  checkToCooked(converter, 0x15555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -17));

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -17));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -17));

  checkToRaw(converter, 0.25, 0x8000);
  checkToRaw(converter, -0.25, 0x38000);
  checkToRaw(converter, 0.75, 0x18000);
  checkToRaw(converter, -0.75, 0x28000);

  // these values are out of range
  checkToRaw(converter, 3.25, 0x1FFFF);
  checkToRaw(converter, -3.25, 0x20000);
  checkToRaw(converter, 5.75, 0x1FFFF);
  checkToRaw(converter, -5.75, 0x20000);
}

BOOST_AUTO_TEST_CASE(testUInt18_fraction17) {
  FixedPointConverter converter("Variable18plus17unsigned", 18, 17,
      false); // 18 bits, 17 fractional bits, not signed

  checkToCooked(converter, 0x2AAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -17));
  checkToCooked(converter, 0x15555, HEX_TO_DOUBLE(0x15555) * pow(2, -17));

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -17));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x15555) * pow(2, -17));

  checkToRaw(converter, 0.25, 0x8000);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x18000);
  checkToRaw(converter, -0.75, 0x0);

  // these values are out of range
  checkToRaw(converter, 3.25, 0x3FFFF);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0x3FFFF);
  checkToRaw(converter, -5.75, 0x0);
}

BOOST_AUTO_TEST_CASE(testInt18_fraction18) {
  FixedPointConverter converter("Variable18plus18signed", 18,
      18); // 18 bits, 18 fractional bits, signed

  checkToCooked(converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -18));
  checkToCooked(converter, 0x15555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -18));

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -18));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -18));

  checkToRaw(converter, 0.25, 0x10000);
  checkToRaw(converter, -0.25, 0x30000);
  // +- 0.75 are out of range
  checkToRaw(converter, 0.75,
      0x1FFFF);                          // the largest possible value (0.5 - 1e-18)
  checkToRaw(converter, -0.75, 0x20000); // -0.5, the smallest possible value

  checkToCooked(converter, 0x10000, 0.25);
  checkToCooked(converter, 0x30000, -0.25);

  // these values are out of range
  checkToRaw(converter, 3.25, 0x1FFFF);
  checkToRaw(converter, -3.25, 0x20000);
  checkToRaw(converter, 5.75, 0x1FFFF);
  checkToRaw(converter, -5.75, 0x20000);
}

BOOST_AUTO_TEST_CASE(testUInt18_fraction18) {
  FixedPointConverter converter("Variable18plus18unsigned", 18, 18,
      false); // 10 bits, -12 fractional bits, not signed

  checkToCooked(converter, 0x2AAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -18));
  checkToCooked(converter, 0x15555, HEX_TO_DOUBLE(0x15555) * pow(2, -18));

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -18));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x15555) * pow(2, -18));

  checkToRaw(converter, 0.25, 0x10000);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x30000);
  checkToRaw(converter, -0.75, 0x0);

  checkToCooked(converter, 0x10000, 0.25);
  checkToCooked(converter, 0x30000, 0.75);

  checkToRaw(converter, 3.25, 0x3FFFF);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0x3FFFF);
  checkToRaw(converter, -5.75, 0x0);
}

BOOST_AUTO_TEST_CASE(testInt18_fraction43) {
  FixedPointConverter converter("int18_fraction43", 18,
      43); // 18 bits, 43 fractional bits, signed

  checkToCooked(converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -43));
  checkToCooked(converter, 0x15555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -43));

  checkToCooked(converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE(0xFFFEAAAA) * pow(2, -43));
  checkToCooked(converter, 0x55555555, SIGNED_HEX_TO_DOUBLE(0x15555) * pow(2, -43));

  // all out of range
  checkToRaw(converter, 0.25, 0x1FFFF);
  checkToRaw(converter, -0.25, 0x20000);
  checkToRaw(converter, 0.75, 0x1FFFF);
  checkToRaw(converter, -0.75, 0x20000);

  checkToRaw(converter, 3.25, 0x1FFFF);
  checkToRaw(converter, -3.25, 0x20000);
  checkToRaw(converter, 5.75, 0x1FFFF);
  checkToRaw(converter, -5.75, 0x20000);
}

BOOST_AUTO_TEST_CASE(testUInt18_fraction43) {
  FixedPointConverter converter("Variable18plus43unsigned", 18, 43,
      false); // 18 bits, -43 fractional bits, not signed

  checkToCooked(converter, 0x2AAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -43));
  checkToCooked(converter, 0x15555, HEX_TO_DOUBLE(0x15555) * pow(2, -43));

  checkToCooked(converter, 0xAAAAAAAA, HEX_TO_DOUBLE(0x2AAAA) * pow(2, -43));
  checkToCooked(converter, 0x55555555, HEX_TO_DOUBLE(0x15555) * pow(2, -43));

  // all out of range
  checkToRaw(converter, 0.25, 0x3FFFF);
  checkToRaw(converter, -0.25, 0x0);
  checkToRaw(converter, 0.75, 0x3FFFF);
  checkToRaw(converter, -0.75, 0x0);

  checkToRaw(converter, 3.25, 0x3FFFF);
  checkToRaw(converter, -3.25, 0x0);
  checkToRaw(converter, 5.75, 0x3FFFF);
  checkToRaw(converter, -5.75, 0x0);
}

BOOST_AUTO_TEST_CASE(testGetters) {
  FixedPointConverter defaultConverter("default");
  BOOST_CHECK(defaultConverter.getNBits() == 32);
  BOOST_CHECK(defaultConverter.getFractionalBits() == 0);
  BOOST_CHECK(defaultConverter.isSigned() == true);

  FixedPointConverter customConverter("custom", 13, 7, false);
  BOOST_CHECK(customConverter.getNBits() == 13);
  BOOST_CHECK(customConverter.getFractionalBits() == 7);
  BOOST_CHECK(customConverter.isSigned() == false);
}

BOOST_AUTO_TEST_CASE(testInt32ToInt32) {
  FixedPointConverter converter("int32toInt32"); // default parameters are signed 32 bit

  checkToCooked(converter, 0, (int32_t)0);
  checkToCooked(converter, 1, (int32_t)1);
  checkToCooked(converter, 0xFFFFFFFF, (int32_t)-1);
  checkToCooked(converter, 3, (int32_t)3);
  checkToCooked(converter, 0xFFFFFFFD, (int32_t)-3);
  checkToCooked(converter, 6, (int32_t)6);
  checkToCooked(converter, 0xFFFFFFFA, (int32_t)-6);
  checkToCooked(converter, 0xAAAAAAAA, (int32_t)0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (int32_t)0x55555555);

  checkToRaw(converter, 0, 0);
  checkToRaw(converter, 1, 1);
  checkToRaw(converter, -1, -1);
  checkToRaw(converter, 3, 3);
  checkToRaw(converter, -3, -3);
  checkToRaw(converter, 6, 6);
  checkToRaw(converter, -6, -6);
}

BOOST_AUTO_TEST_CASE(testInt32ToInt16) {
  FixedPointConverter converter("int32ToInt16"); // default constructor is signed 32 bit

  checkToCooked(converter, 0, (int16_t)0);
  checkToCooked(converter, 1, (int16_t)1);
  checkToCooked(converter, 0xFFFFFFFF, (int16_t)-1);
  checkToCooked(converter, 3, (int16_t)3);
  checkToCooked(converter, 0xFFFFFFFD, (int16_t)-3);
  checkToCooked(converter, 6, (int16_t)6);
  checkToCooked(converter, 0xFFFFFFFA, (int16_t)-6);
  checkToCookedOverflowNeg<int16_t>(converter, 0xAAAAAAAA);
  checkToCookedOverflowPos<int16_t>(converter, 0x55555555);

  checkToRaw(converter, (int16_t)0, 0);
  checkToRaw(converter, (int16_t)1, 1);
  checkToRaw(converter, (int16_t)-1, -1);
  checkToRaw(converter, (int16_t)3, 3);
  checkToRaw(converter, (int16_t)-3, -3);
  checkToRaw(converter, (int16_t)6, 6);
  checkToRaw(converter, (int16_t)-6, -6);
  checkToRaw(converter, (int16_t)0x7FFF, 0x7FFF);
  checkToRaw(converter, (int16_t)0x8000, 0xFFFF8000);
  checkToRaw(converter, (int16_t)0xFFFF, 0xFFFFFFFF);
}

BOOST_AUTO_TEST_CASE(testInt18ToInt32) {
  FixedPointConverter converter("int18ToInt32", 18, 0, true);

  checkToCooked(converter, 0, 0);
  checkToCooked(converter, 1, 1);
  checkToCooked(converter, 0x3FFFF, -1);
  checkToCooked(converter, 3, 3);
  checkToCooked(converter, 0x3FFFD, -3);
  checkToCooked(converter, 6, 6);
  checkToCooked(converter, 0x3FFFA, -6);
  checkToCooked(converter, 0xFFFFFFFF, (int32_t)0xFFFFFFFF);
  checkToCooked(converter, 0xFFFFFFFE, (int32_t)0xFFFFFFFE);
  checkToCooked(converter, 0x55555555, (int32_t)0x15555);

  checkToRaw(converter, (int32_t)0, 0);
  checkToRaw(converter, (int32_t)1, 1);
  checkToRaw(converter, (int32_t)-1, 0x3FFFF);
  checkToRaw(converter, (int32_t)3, 3);
  checkToRaw(converter, (int32_t)-3, 0x3FFFD);
  checkToRaw(converter, (int32_t)6, 6);
  checkToRaw(converter, (int32_t)-6, 0x3FFFA);
  checkToRaw(converter, (int32_t)0x1FFFF, 0x1FFFF);
  checkToRaw(converter, (int32_t)0x20000, 0x1FFFF);
  checkToRaw(converter, (int32_t)-1, 0x3FFFF);
  checkToRaw(converter, (int32_t)-0x20000, 0x20000);
}

BOOST_AUTO_TEST_CASE(testIntSignedToUnsigned) {
  FixedPointConverter converter("signedToUnsigned"); // default parameters are signed 32 bit

  checkToCooked(converter, 0, (uint32_t)0);
  checkToCooked(converter, 1, (uint32_t)1);
  checkToCooked(converter, 3, (uint32_t)3);
  checkToCooked(converter, 6, (uint32_t)6);
  checkToCookedOverflowNeg<uint32_t>(converter, 0xFFFFFFFF);
  checkToCookedOverflowNeg<uint16_t>(converter, 0xFFFFFFFA);
  checkToCookedOverflowNeg<uint16_t>(converter, 0xAAAAAAAA);
  checkToCooked(converter, 0x55555555, (uint32_t)0x55555555);

  checkToRaw(converter, (uint32_t)0, 0);
  checkToRaw(converter, (uint32_t)1, 1);
  checkToRaw(converter, (uint32_t)3, 3);
  checkToRaw(converter, (uint32_t)6, 6);
  checkToRaw(converter, (uint32_t)0x7FFFFFFF, 0x7FFFFFFF);
  checkToRaw(converter, (uint32_t)0x80000000, 0x7FFFFFFF);
  checkToRaw(converter, (uint32_t)0xFFFFFFFF, 0x7FFFFFFF);
}

BOOST_AUTO_TEST_CASE(testInt17SignedToInt16Unsigned) {
  FixedPointConverter converter("int17SignedToInt16Unsigned", 17, 0, true);

  checkToCooked(converter, 0, (uint16_t)0);
  checkToCooked(converter, 1, (uint16_t)1);
  checkToCookedOverflowNeg<uint16_t>(converter, 0xFFFFFFFF);
  checkToCooked(converter, 3, (uint16_t)3);
  checkToCooked(converter, 6, (uint16_t)6);
  checkToCooked(converter, 0xAAAAAAAA, (uint16_t)0xAAAA);
  checkToCookedOverflowNeg<int16_t>(converter, 0x55555555);

  checkToRaw(converter, (uint16_t)0, 0);
  checkToRaw(converter, (uint16_t)1, 1);
  checkToRaw(converter, (uint16_t)3, 3);
  checkToRaw(converter, (uint16_t)6, 6);
  checkToRaw(converter, (uint16_t)0x7FFF, 0x7FFF);
  checkToRaw(converter, (uint16_t)0x8000, 0x8000);
  checkToRaw(converter, (uint16_t)0xFFFF, 0xFFFF);
}

BOOST_AUTO_TEST_CASE(testInt0unsigned) { // test with 0 significant bits
                                         // (unsigned, no fractional bits)
  FixedPointConverter converter("int0unsigned", 0, 0, false);

  checkToCooked(converter, 0, 0);
  checkToCooked(converter, 1, 0);
  checkToCooked(converter, 0x0000FFFF, 0);
  checkToCooked(converter, 0xFFFF0000, 0);
  checkToCooked(converter, 0xFFFFFFFF, 0);

  checkToRaw(converter, 0, 0);
  checkToRaw(converter, 1, 0);
  checkToRaw(converter, 0xFFFF, 0);
  checkToRaw(converter, -1, 0);
}

BOOST_AUTO_TEST_CASE(testInt0signed) { // test with 0 significant bits (signed,
                                       // no fractional bits)
  FixedPointConverter converter("int0signed", 0, 0, true);

  checkToCooked(converter, 0, 0);
  checkToCooked(converter, 1, 0);
  checkToCooked(converter, 0x0000FFFF, 0);
  checkToCooked(converter, 0xFFFF0000, 0);
  checkToCooked(converter, 0xFFFFFFFF, 0);

  checkToRaw(converter, 0, 0);
  checkToRaw(converter, 1, 0);
  checkToRaw(converter, 0xFFFF, 0);
  checkToRaw(converter, -1, 0);
}

BOOST_AUTO_TEST_CASE(testInt0unsignedFractional) { // test with 0 significant bits (unsigned,
                                                   // with fractional bits)
  FixedPointConverter converter("int0unsignedFractional", 0, 5, false);

  checkToCooked(converter, 0, 0);
  checkToCooked(converter, 1, 0);
  checkToCooked(converter, 0x0000FFFF, 0);
  checkToCooked(converter, 0xFFFF0000, 0);
  checkToCooked(converter, 0xFFFFFFFF, 0);

  checkToRaw(converter, 0, 0);
  checkToRaw(converter, 1, 0);
  checkToRaw(converter, 0xFFFF, 0);
  checkToRaw(converter, -1, 0);
}

BOOST_AUTO_TEST_CASE(testInt0signedFractional) { // test with 0 significant bits (signed, with
                                                 // negative fractional bits)
  FixedPointConverter converter("int0signedFractional", 0, -5, true);

  checkToCooked(converter, 0, 0);
  checkToCooked(converter, 1, 0);
  checkToCooked(converter, 0x0000FFFF, 0);
  checkToCooked(converter, 0xFFFF0000, 0);
  checkToCooked(converter, 0xFFFFFFFF, 0);

  checkToRaw(converter, 0, 0);
  checkToRaw(converter, 1, 0);
  checkToRaw(converter, 0xFFFF, 0);
  checkToRaw(converter, -1, 0);
}

BOOST_AUTO_TEST_CASE(testDynamicRangePos) {
  FixedPointConverter converter("dynamicRangePos", 16, 1021 - 16, false);

  checkToCooked(converter, 0, 0.);
  checkToCooked(converter, 1, pow(2., -(1021 - 16)));
  checkToCooked(converter, 0xFFFF, 65535. * pow(2., -(1021 - 16)));

  // check that our comparison values are not already exceeding the dynamic
  // range
  BOOST_CHECK(!std::isinf(65535. * pow(2., -(1021 - 16))));
}

BOOST_AUTO_TEST_CASE(testDynamicRangeNeg) {
  FixedPointConverter converter("dynamicRangeNeg", 16, -1024 + 16, false);

  checkToCooked(converter, 0, 0.);
  checkToCooked(converter, 1, pow(2., 1024 - 16));
  checkToCooked(converter, 0xFFFF, 65535. * pow(2., 1024 - 16));

  // check that our comparison values are not already exceeding the dynamic
  // range
  BOOST_CHECK(pow(2., -(1024 - 16)) > 0.);
}

BOOST_AUTO_TEST_SUITE_END()
