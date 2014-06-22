/*  Test requirements:
 *  Test to and from double for the follwing cases:
 *  int32, uint32, int16, uint16, int8, uint8. No fractional bits (standard data types)
 *  32 bits with -12 (negative), -1 (test rounding), 1 (test rounding),
 *  7 (somewhere in the middle), 31, 32 (resolution edge) and 43 (larger than 32 bits),
 *  fractional bits, signed and unsigned
 *  18 bits with -12, 0, 7, 17, 18, 43 fractional bits, signed and unsigned
 *
 *  All tests are run with the bit sequence 0xAAAAAAAA (negative when signed)
 *  and 0x55555555 (positive when signed) to float,
 *  and with +-0.25, +-0.75, +-3.25 +-5.75 to fixed
 *  to test correct rounding.
 *
 */

#define HEX_TO_DOUBLE( INPUT ) static_cast<double>( INPUT )
#define SIGNED_HEX_TO_DOUBLE( INPUT ) static_cast<double>( static_cast<int32_t>(INPUT) )

#define CHECK_SIGNED_FIXED_POINT_NO_FRACTIONAL\
  checkToFixedPoint( converter, 0.25, 0 );\
  checkToFixedPoint( converter, -0.25, 0 );\
  checkToFixedPoint( converter, 0.75, 1 );\
  checkToFixedPoint( converter, -0.75, -1 );\
  checkToFixedPoint( converter, 3.25, 3 );\
  checkToFixedPoint( converter, -3.25, -3 );\
  checkToFixedPoint( converter, 5.75, 6 );\
  checkToFixedPoint( converter, -5.75, -6 );


#define BOOST_TEST_MODULE FixedPointConverterTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <sstream>
#include <stdexcept>

#include "FixedPointConverter.h"
using namespace mtca4u;

void checkToDouble(FixedPointConverter const & converter,
		   int32_t input, double expectedValue){
  double result = converter.toDouble( input );

  std::stringstream message;
  message << "testToDouble failed for input " << std::hex << "0x" << input
	  << ", output " << result
	  << ", expected " << expectedValue
	  << std::dec;
  
  BOOST_CHECK_MESSAGE( result == expectedValue, message.str() );

}

void checkToFixedPoint(FixedPointConverter const & converter,
		       double input, int32_t expectedValue){
  int32_t result = converter.toFixedPoint( input );

  std::stringstream message;
  message << "testToFixedPoint failed for input " << std::hex << input
	  << ", output 0x" << result
	  << ", expected 0x" << expectedValue
	  << std::dec;
  
  BOOST_CHECK_MESSAGE( result == expectedValue, message.str() );
}

BOOST_AUTO_TEST_SUITE( FixedPointConverterTestSuite )

BOOST_AUTO_TEST_CASE( testConstructor ){
  BOOST_CHECK_NO_THROW( FixedPointConverter() );
  BOOST_CHECK_NO_THROW( FixedPointConverter(16, 42, false) );
  BOOST_CHECK_THROW( FixedPointConverter(33), std::invalid_argument);
  BOOST_CHECK_THROW( FixedPointConverter(32, 5000) , std::invalid_argument);
  BOOST_CHECK_THROW( FixedPointConverter(32, -5000) , std::invalid_argument);
  BOOST_CHECK_THROW( FixedPointConverter(0) , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE( testInt32 ){
  FixedPointConverter converter; // default constructor is signed 32 bit
  
  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555) );
  
  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, -1 );
  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, -3 );
  checkToFixedPoint( converter, 5.75, 6 );
  checkToFixedPoint( converter, -5.75, -6 );
}

BOOST_AUTO_TEST_CASE( testUInt32 ){
  FixedPointConverter converter(32, 0, false); // 32 bits, 0 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, 0xFFFFFFFF );
  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFFFD );
  checkToFixedPoint( converter, 5.75, 6 );
  checkToFixedPoint( converter, -5.75, 0xFFFFFFFA );
}

BOOST_AUTO_TEST_CASE( testInt16 ){
  FixedPointConverter converter(16); // 16 bits, 0 fractional bits, signed
  
  checkToDouble( converter, 0xAAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFFAAAA) );
  checkToDouble( converter, 0x5555, HEX_TO_DOUBLE( 0x5555) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, 0xFFFF );
  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0xFFFD );
  checkToFixedPoint( converter, 5.75, 6 );
  checkToFixedPoint( converter, -5.75, 0xFFFA );
}

BOOST_AUTO_TEST_CASE( testUInt16 ){
  FixedPointConverter converter(16, 0, false); // 16 bits, 0 fractional bits, not signed

  checkToDouble( converter, 0xAAAA, HEX_TO_DOUBLE( 0xAAAA) );
  checkToDouble( converter, 0x5555, HEX_TO_DOUBLE( 0x5555) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, 0xFFFF );
  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0xFFFD );
  checkToFixedPoint( converter, 5.75, 6 );
  checkToFixedPoint( converter, -5.75, 0xFFFA );
}

BOOST_AUTO_TEST_CASE( testInt8 ){
  FixedPointConverter converter(8); // 8 bits, 0 fractional bits, signed
  
  checkToDouble( converter, 0xAA, SIGNED_HEX_TO_DOUBLE( 0xFFFFFFAA) );
  checkToDouble( converter, 0x55, HEX_TO_DOUBLE( 0x55) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, 0xFF );
  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0xFD );
  checkToFixedPoint( converter, 5.75, 6 );
  checkToFixedPoint( converter, -5.75, 0xFA );
}

BOOST_AUTO_TEST_CASE( testUInt8 ){
  FixedPointConverter converter(8, 0, false); // 8 bits, 0 fractional bits, not signed

  checkToDouble( converter, 0xAA, HEX_TO_DOUBLE( 0xAA) );
  checkToDouble( converter, 0x55, HEX_TO_DOUBLE( 0x55) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, 0xFF );
  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0xFD );
  checkToFixedPoint( converter, 5.75, 6 );
  checkToFixedPoint( converter, -5.75, 0xFA );
}

BOOST_AUTO_TEST_CASE( testInt32_fractionMinus12 ){
  FixedPointConverter converter(32, -12); // 32 bits, -12 fractional bits, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA) * pow(2,12) );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555) * pow(2,12));

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75, 0 );
  checkToFixedPoint( converter, 3.25, 0 );
  checkToFixedPoint( converter, -3.25, 0 );
  checkToFixedPoint( converter, 5.75, 0 );
  checkToFixedPoint( converter, -5.75, 0 );
}

BOOST_AUTO_TEST_CASE( testUInt32_fractionMinus12 ){
  FixedPointConverter converter(32, -12, false); // 32 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA) * pow(2,12) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555) * pow(2,12));

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75, 0 );
  checkToFixedPoint( converter, 3.25, 0 );
  checkToFixedPoint( converter, -3.25, 0 );
  checkToFixedPoint( converter, 5.75, 0 );
  checkToFixedPoint( converter, -5.75, 0 );
}

BOOST_AUTO_TEST_CASE( testInt32_fractionMinus1 ){
  FixedPointConverter converter(32, -1); // 32 bits, -12 fractional bits, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA) * 2 );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555) * 2 );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75, 0 );

  // bit pattern of 3 is 11, where the last digit is rounded up, and afterwards 
  // one bit is shifted. So the actual value is 4
  checkToFixedPoint( converter, 3.25, 0x2 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFFFE ); // (-2)
  checkToFixedPoint( converter, 5.75, 0x3 ); 
  checkToFixedPoint( converter, -5.75, 0xFFFFFFFD ); // (-3)
}

BOOST_AUTO_TEST_CASE( testUInt32_fractionMinus1 ){
  FixedPointConverter converter(32, -1, false); // 32 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA) * 2 );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555) * 2 );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75, 0 );

  // bit pattern of 3 is 11, where the last digit is rounded up, and afterwards 
  // one bit is shifted. So the actual value is 4
  checkToFixedPoint( converter, 3.25, 0x2 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFFFE ); // (-2)
  checkToFixedPoint( converter, 5.75, 0x3 ); 
  checkToFixedPoint( converter, -5.75, 0xFFFFFFFD ); // (-3)
}

BOOST_AUTO_TEST_CASE( testInt32_fraction1 ){
  FixedPointConverter converter(32, 1); // 32 bits, 1 fractional bit, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA) * 0.5 );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555) * 0.5 );

  checkToFixedPoint( converter, 0.25, 0x1 );
  checkToFixedPoint( converter, -0.25, 0xFFFFFFFF );
  checkToFixedPoint( converter, 0.75, 0x2 );
  checkToFixedPoint( converter, -0.75, 0xFFFFFFFE );

  checkToFixedPoint( converter, 3.25, 0x7 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFFF9 ); // (-7)
  checkToFixedPoint( converter, 5.75, 0xC ); 
  checkToFixedPoint( converter, -5.75, 0xFFFFFFF4 ); // (-12)
}

BOOST_AUTO_TEST_CASE( testUInt32_fraction1 ){
  FixedPointConverter converter(32, 1, false); // 32 bits, 1 fractional bit, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA) * 0.5 );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555) * 0.5 );

  checkToFixedPoint( converter, 0.25, 0x1 );
  checkToFixedPoint( converter, -0.25, 0xFFFFFFFF );
  checkToFixedPoint( converter, 0.75, 0x2 );
  checkToFixedPoint( converter, -0.75, 0xFFFFFFFE );

  checkToFixedPoint( converter, 3.25, 0x7 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFFF9 ); // (-7)
  checkToFixedPoint( converter, 5.75, 0xC ); 
  checkToFixedPoint( converter, -5.75, 0xFFFFFFF4 ); // (-12)
}

BOOST_AUTO_TEST_CASE( testInt32_fraction7 ){
  FixedPointConverter converter(32,7); // 32 bits, 7 fractional bits, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-7) );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555)*pow(2,-7) );

  checkToFixedPoint( converter, 0.25, 0x20 );
  checkToFixedPoint( converter, -0.25, 0xFFFFFFE0 );
  checkToFixedPoint( converter, 0.75, 0x60 );
  checkToFixedPoint( converter, -0.75,  0xFFFFFFA0 );

  checkToFixedPoint( converter, 3.25, 0x1A0 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFE60 );
  checkToFixedPoint( converter, 5.75, 0x2E0 ); 
  checkToFixedPoint( converter, -5.75, 0xFFFFFD20 ); 
}

BOOST_AUTO_TEST_CASE( testUInt32_fraction7 ){
  FixedPointConverter converter(32, 7, false); // 32 bits, -7 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-7) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555)*pow(2,-7) );

  checkToFixedPoint( converter, 0.25, 0x20 );
  checkToFixedPoint( converter, -0.25, 0xFFFFFFE0 );
  checkToFixedPoint( converter, 0.75, 0x60 );
  checkToFixedPoint( converter, -0.75,  0xFFFFFFA0 );

  checkToFixedPoint( converter, 3.25, 0x1A0 );
  checkToFixedPoint( converter, -3.25, 0xFFFFFE60 );
  checkToFixedPoint( converter, 5.75, 0x2E0 ); 
  checkToFixedPoint( converter, -5.75, 0xFFFFFD20 ); 
}

BOOST_AUTO_TEST_CASE( testInt32_fraction31 ){
  FixedPointConverter converter(32,31); // 18 bits, 31 fractional bits, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-31) );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555)*pow(2,-31) );

  checkToFixedPoint( converter, 0.25, 0x20000000 );
  checkToFixedPoint( converter, -0.25, 0xE0000000 );
  checkToFixedPoint( converter, 0.75, 0x60000000 );
  checkToFixedPoint( converter, -0.75, 0xA0000000 );

  checkToFixedPoint( converter, 3.25, 0xA0000000 );
  checkToFixedPoint( converter, -3.25, 0x60000000 );
  checkToFixedPoint( converter, 5.75, 0xE0000000 ); 
  checkToFixedPoint( converter, -5.75, 0x20000000 ); 
  // correct interpretration is +-0.25, +-0.75
  checkToDouble( converter, 0xA0000000, -0.75 );
  checkToDouble( converter, 0x60000000,  0.75 );
  checkToDouble( converter, 0xE0000000, -0.25 );
  checkToDouble( converter, 0x20000000,  0.25 );
}

BOOST_AUTO_TEST_CASE( testUInt32_fraction31 ){
  FixedPointConverter converter(32, 31, false); // 32 bits, 31 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-31) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555)*pow(2,-31) );

  checkToFixedPoint( converter, 0.25, 0x20000000 );
  checkToFixedPoint( converter, -0.25, 0xE0000000 );
  checkToFixedPoint( converter, 0.75, 0x60000000 );
  checkToFixedPoint( converter, -0.75, 0xA0000000 );

  checkToFixedPoint( converter, 3.25, 0xA0000000 );
  checkToFixedPoint( converter, -3.25, 0x60000000 );
  checkToFixedPoint( converter, 5.75, 0xE0000000 ); 
  checkToFixedPoint( converter, -5.75, 0x20000000 ); 
  // correct interpretration is 0.25, to 1.75
  checkToDouble( converter, 0xA0000000,  1.25 );
  checkToDouble( converter, 0x60000000,  0.75 );
  checkToDouble( converter, 0xE0000000,  1.75 );
  checkToDouble( converter, 0x20000000,  0.25 );
}

BOOST_AUTO_TEST_CASE( testInt32_fraction32 ){
  FixedPointConverter converter(32,32); // 18 bits, 32 fractional bits, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-32) );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555)*pow(2,-32) );

  checkToFixedPoint( converter,  0.25, 0x40000000 );
  checkToFixedPoint( converter, -0.25, 0xC0000000 );
  checkToFixedPoint( converter,  0.75, 0xC0000000 );
  checkToFixedPoint( converter, -0.75, 0x40000000 );

  checkToFixedPoint( converter,  3.25, 0x40000000 );
  checkToFixedPoint( converter, -3.25, 0xC0000000 );
  checkToFixedPoint( converter,  5.75, 0xC0000000 ); 
  checkToFixedPoint( converter, -5.75, 0x40000000 );
  // correct interpretration is +-0.25
  checkToDouble( converter, 0x40000000,  0.25 );
  checkToDouble( converter, 0xC0000000, -0.25  );
}

BOOST_AUTO_TEST_CASE( testUInt32_fraction32 ){
  FixedPointConverter converter(32, 32, false); // 32 bits, -32 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-32) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555)*pow(2,-32) );

  checkToFixedPoint( converter,  0.25, 0x40000000 );
  checkToFixedPoint( converter, -0.25, 0xC0000000 );
  checkToFixedPoint( converter,  0.75, 0xC0000000 );
  checkToFixedPoint( converter, -0.75, 0x40000000 );

  checkToFixedPoint( converter,  3.25, 0x40000000 );
  checkToFixedPoint( converter, -3.25, 0xC0000000 );
  checkToFixedPoint( converter,  5.75, 0xC0000000 ); 
  checkToFixedPoint( converter, -5.75, 0x40000000 );
  // correct interpretration is 0.25, 0.75
  checkToDouble( converter, 0x40000000,  0.25 );
  checkToDouble( converter, 0xC0000000,  0.75  );
}

BOOST_AUTO_TEST_CASE( testInt32_fraction43 ){
  FixedPointConverter converter(32,43); // 18 bits, 43 fractional bits, signed

  checkToDouble( converter, 0xAAAAAAAA, SIGNED_HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-43) );
  checkToDouble( converter, 0x55555555, SIGNED_HEX_TO_DOUBLE( 0x55555555)*pow(2,-43) );

  // way out of the sensisive region (the values are all exact multiples of 1/4, but 
  // sensitivity is out of range at O(1e-3) )
  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75,  0 );

  checkToFixedPoint( converter, 3.25, 0 );
  checkToFixedPoint( converter, -3.25, 0 );
  checkToFixedPoint( converter, 5.75, 0 ); 
  checkToFixedPoint( converter, -5.75, 0 ); 
}

BOOST_AUTO_TEST_CASE( testUInt32_fraction43 ){
  FixedPointConverter converter(32, 43, false); // 32 bits, -43 fractional bits, not signed

  checkToDouble( converter, 0xAAAAAAAA, HEX_TO_DOUBLE( 0xAAAAAAAA)*pow(2,-43) );
  checkToDouble( converter, 0x55555555, HEX_TO_DOUBLE( 0x55555555)*pow(2,-43) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75,  0 );

  checkToFixedPoint( converter, 3.25, 0x0 );
  checkToFixedPoint( converter, -3.25, 0x0 );
  checkToFixedPoint( converter, 5.75, 0x0 ); 
  checkToFixedPoint( converter, -5.75, 0x0 ); 
}

BOOST_AUTO_TEST_CASE( testInt18_fractionMinus12 ){
  FixedPointConverter converter(18, -12); // 10 bits, -12 fractional bits, signed

  checkToDouble( converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFEAAAA) * pow(2,12) );
  checkToDouble( converter, 0x15555, SIGNED_HEX_TO_DOUBLE( 0x15555) * pow(2,12) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75, 0 );

  checkToFixedPoint( converter, 3.25, 0 );
  checkToFixedPoint( converter, -3.25, 0 );
  checkToFixedPoint( converter, 5.75, 0 ); 
  checkToFixedPoint( converter, -5.75, 0); 
}

BOOST_AUTO_TEST_CASE( testUInt18_fractionMinus12 ){
  FixedPointConverter converter(18, -12, false); // 10 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0x2AAAA, HEX_TO_DOUBLE( 0x2AAAA) * pow(2,12) );
  checkToDouble( converter, 0x15555, HEX_TO_DOUBLE( 0x15555) * pow(2,12) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75, 0 );

  checkToFixedPoint( converter, 3.25, 0 );
  checkToFixedPoint( converter, -3.25, 0 );
  checkToFixedPoint( converter, 5.75, 0 ); 
  checkToFixedPoint( converter, -5.75, 0); 
}

BOOST_AUTO_TEST_CASE( testInt18_fraction0 ){
  FixedPointConverter converter(18); // 18 bits, 0 fractional bits, signed

  checkToDouble( converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFEAAAA) );
  checkToDouble( converter, 0x15555, SIGNED_HEX_TO_DOUBLE( 0x15555) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75,  0x3FFFF );

  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0x3FFFD );
  checkToFixedPoint( converter, 5.75, 6 ); 
  checkToFixedPoint( converter, -5.75, 0x3FFFA ); 
}

BOOST_AUTO_TEST_CASE( testUInt18_fraction0 ){
  FixedPointConverter converter(18, 0, false); // 10 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0x2AAAA, HEX_TO_DOUBLE( 0x2AAAA) );
  checkToDouble( converter, 0x15555, HEX_TO_DOUBLE( 0x15555) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 1 );
  checkToFixedPoint( converter, -0.75, 0x3FFFF );

  checkToFixedPoint( converter, 3.25, 3 );
  checkToFixedPoint( converter, -3.25, 0x3FFFD );
  checkToFixedPoint( converter, 5.75, 6 ); 
  checkToFixedPoint( converter, -5.75, 0x3FFFA); 
}

BOOST_AUTO_TEST_CASE( testInt18_fraction7 ){
  FixedPointConverter converter(18,7); // 18 bits, 7 fractional bits, signed

  checkToDouble( converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFEAAAA)*pow(2,-7) );
  checkToDouble( converter, 0x15555, SIGNED_HEX_TO_DOUBLE( 0x15555)*pow(2,-7) );

  checkToFixedPoint( converter, 0.25, 0x20 );
  checkToFixedPoint( converter, -0.25, 0x3FFE0 );
  checkToFixedPoint( converter, 0.75, 0x60 );
  checkToFixedPoint( converter, -0.75,  0x3FFA0 );

  checkToFixedPoint( converter, 3.25, 0x1A0 );
  checkToFixedPoint( converter, -3.25, 0x3FE60 );
  checkToFixedPoint( converter, 5.75, 0x2E0 ); 
  checkToFixedPoint( converter, -5.75, 0x3FD20 ); 
}

BOOST_AUTO_TEST_CASE( testUInt18_fraction7 ){
  FixedPointConverter converter(18, 7, false); // 10 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0x2AAAA, HEX_TO_DOUBLE( 0x2AAAA)*pow(2,-7) );
  checkToDouble( converter, 0x15555, HEX_TO_DOUBLE( 0x15555)*pow(2,-7) );

  checkToFixedPoint( converter, 0.25, 0x20 );
  checkToFixedPoint( converter, -0.25, 0x3FFE0 );
  checkToFixedPoint( converter, 0.75, 0x60 );
  checkToFixedPoint( converter, -0.75,  0x3FFA0 );

  checkToFixedPoint( converter, 3.25, 0x1A0 );
  checkToFixedPoint( converter, -3.25, 0x3FE60 );
  checkToFixedPoint( converter, 5.75, 0x2E0 ); 
  checkToFixedPoint( converter, -5.75, 0x3FD20 ); 
}

BOOST_AUTO_TEST_CASE( testInt18_fraction17 ){
  FixedPointConverter converter(18,17); // 18 bits, 17 fractional bits, signed

  checkToDouble( converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFEAAAA)*pow(2,-17) );
  checkToDouble( converter, 0x15555, SIGNED_HEX_TO_DOUBLE( 0x15555)*pow(2,-17) );

  checkToFixedPoint( converter, 0.25, 0x8000 );
  checkToFixedPoint( converter, -0.25, 0x38000 );
  checkToFixedPoint( converter, 0.75, 0x18000 );
  checkToFixedPoint( converter, -0.75,  0x28000 );

  checkToFixedPoint( converter, 3.25, 0x28000 );
  checkToFixedPoint( converter, -3.25, 0x18000 );
  checkToFixedPoint( converter, 5.75, 0x38000 ); 
  checkToFixedPoint( converter, -5.75, 0x08000 ); 
}

BOOST_AUTO_TEST_CASE( testUInt18_fraction17 ){
  FixedPointConverter converter(18, 17, false); // 10 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0x2AAAA, HEX_TO_DOUBLE( 0x2AAAA)*pow(2,-17) );
  checkToDouble( converter, 0x15555, HEX_TO_DOUBLE( 0x15555)*pow(2,-17) );

  checkToFixedPoint( converter, 0.25, 0x8000 );
  checkToFixedPoint( converter, -0.25, 0x38000 );
  checkToFixedPoint( converter, 0.75, 0x18000 );
  checkToFixedPoint( converter, -0.75,  0x28000 );

  checkToFixedPoint( converter, 3.25, 0x28000 );
  checkToFixedPoint( converter, -3.25, 0x18000 );
  checkToFixedPoint( converter, 5.75, 0x38000 ); 
  checkToFixedPoint( converter, -5.75, 0x08000 ); 
}

BOOST_AUTO_TEST_CASE( testInt18_fraction18 ){
  FixedPointConverter converter(18,18); // 18 bits, 17 fractional bits, signed

  checkToDouble( converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFEAAAA)*pow(2,-18) );
  checkToDouble( converter, 0x15555, SIGNED_HEX_TO_DOUBLE( 0x15555)*pow(2,-18) );

  checkToFixedPoint( converter, 0.25, 0x10000 );
  checkToFixedPoint( converter, -0.25, 0x30000 );
  checkToFixedPoint( converter, 0.75, 0x30000 );
  checkToFixedPoint( converter, -0.75,  0x10000 );
  // -0.25 and 0.75 are the same hex value, as are 0.25 and -0.75. The +-0.75 values
  // are truncated, correct interpretation is +-0.25
  checkToDouble( converter, 0x10000, 0.25 );
  checkToDouble( converter, 0x30000, -0.25 );

  checkToFixedPoint( converter, 3.25, 0x10000 );
  checkToFixedPoint( converter, -3.25, 0x30000 );
  checkToFixedPoint( converter, 5.75, 0x30000 ); 
  checkToFixedPoint( converter, -5.75, 0x10000 ); 
}

BOOST_AUTO_TEST_CASE( testUInt18_fraction18 ){
  FixedPointConverter converter(18, 18, false); // 10 bits, -12 fractional bits, not signed

  checkToDouble( converter, 0x2AAAA, HEX_TO_DOUBLE( 0x2AAAA)*pow(2,-18) );
  checkToDouble( converter, 0x15555, HEX_TO_DOUBLE( 0x15555)*pow(2,-18) );

  checkToFixedPoint( converter, 0.25, 0x10000 );
  checkToFixedPoint( converter, -0.25, 0x30000 );
  checkToFixedPoint( converter, 0.75, 0x30000 );
  checkToFixedPoint( converter, -0.75,  0x10000 );
  // -0.25 and 0.75 are the same hex value, as are 0.25 and -0.75. The +-0.75 values
  // are truncated, correct interpretation is 0.25 and 0.75
  checkToDouble( converter, 0x10000, 0.25 );
  checkToDouble( converter, 0x30000, 0.75 );

  checkToFixedPoint( converter, 3.25, 0x10000 );
  checkToFixedPoint( converter, -3.25, 0x30000 );
  checkToFixedPoint( converter, 5.75, 0x30000 ); 
  checkToFixedPoint( converter, -5.75, 0x10000 ); 
}

BOOST_AUTO_TEST_CASE( testInt18_fraction43 ){
  FixedPointConverter converter(18,43); // 18 bits, 43 fractional bits, signed

  checkToDouble( converter, 0x2AAAA, SIGNED_HEX_TO_DOUBLE( 0xFFFEAAAA)*pow(2,-43) );
  checkToDouble( converter, 0x15555, SIGNED_HEX_TO_DOUBLE( 0x15555)*pow(2,-43) );

  // way out of the sensisive region (the values are all exact multiples of 1/4, but 
  // sensitivity is out of range at O(3e-6) )
  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75,  0 );

  checkToFixedPoint( converter, 3.25, 0 );
  checkToFixedPoint( converter, -3.25, 0 );
  checkToFixedPoint( converter, 5.75, 0 ); 
  checkToFixedPoint( converter, -5.75, 0 ); 
}

BOOST_AUTO_TEST_CASE( testUInt18_fraction43 ){
  FixedPointConverter converter(18, 43, false); // 18 bits, -43 fractional bits, not signed

  checkToDouble( converter, 0x2AAAA, HEX_TO_DOUBLE( 0x2AAAA)*pow(2,-43) );
  checkToDouble( converter, 0x15555, HEX_TO_DOUBLE( 0x15555)*pow(2,-43) );

  checkToFixedPoint( converter, 0.25, 0 );
  checkToFixedPoint( converter, -0.25, 0 );
  checkToFixedPoint( converter, 0.75, 0 );
  checkToFixedPoint( converter, -0.75,  0 );

  checkToFixedPoint( converter, 3.25, 0x0 );
  checkToFixedPoint( converter, -3.25, 0x0 );
  checkToFixedPoint( converter, 5.75, 0x0 ); 
  checkToFixedPoint( converter, -5.75, 0x0 ); 
}

BOOST_AUTO_TEST_CASE( testSetParameters ){
  // start with the default constructor
  FixedPointConverter converter;
  checkToDouble( converter, 0xFFFFFFEF, -17.);

  converter.setParameters(32, 0, false);
  checkToDouble( converter, 0xFFFFFFEF, 4294967279.);

  converter.setParameters(8, 3, true);
  checkToDouble( converter, 0xEF, -2.125);
}


BOOST_AUTO_TEST_SUITE_END()
