#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FloatRawDataTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

/* This test is checking that IEE754 encoded single precision floats (32 bits)
 * are transferred corretly to and from int32_t raw registers.
 */

#include "Device.h"
using namespace ChimeraTK;

// BOOST_AUTO_TEST_CASE( testCatalogueEntries ) {
//  BOOST_CHECK(false);
//}

BOOST_AUTO_TEST_CASE(testReading) {
  Device d;
  d.open("(dummy?map=floatRawTest.map)");

  // take the back door and use int register which points to the same memory
  // space. Raw accessors are not supported (yet?).
  auto rawAccessor = d.getScalarRegisterAccessor<int32_t>("FLOAT_TEST/SCALAR_AS_INT", 0, {AccessMode::raw});
  rawAccessor = 0x40700000; // IEE754 bit representation of 3.75
  rawAccessor.write();

  auto floatAccessor = d.getScalarRegisterAccessor<float>("FLOAT_TEST/SCALAR");
  floatAccessor.read();
  BOOST_CHECK_CLOSE(float(floatAccessor), 3.75, 0.0001);

  auto doubleAccessor = d.getScalarRegisterAccessor<double>("FLOAT_TEST/SCALAR");
  doubleAccessor.read();
  BOOST_CHECK_CLOSE(double(doubleAccessor), 3.75, 0.0001);

  auto intAccessor = d.getScalarRegisterAccessor<int32_t>("FLOAT_TEST/SCALAR");
  intAccessor.read();
  BOOST_CHECK_EQUAL(int32_t(intAccessor), 4);

  auto stringAccessor = d.getScalarRegisterAccessor<std::string>("FLOAT_TEST/SCALAR");
  stringAccessor.read();
  BOOST_CHECK_EQUAL(std::string(stringAccessor), std::to_string(3.75));
}

void checkAsRaw(int32_t rawValue, float expectedValue) {
  void* warningAvoider = &rawValue;
  float testValue = *(reinterpret_cast<float*>(warningAvoider));

  BOOST_CHECK_CLOSE(testValue, expectedValue, 0.0001);
}

BOOST_AUTO_TEST_CASE(testWriting) {
  Device d;
  d.open("(dummy?map=floatRawTest.map)");

  auto floatAccessor = d.getOneDRegisterAccessor<float>("FLOAT_TEST/ARRAY");
  floatAccessor[0] = 1.23;
  floatAccessor[1] = 2.23;
  floatAccessor[2] = 3.23;
  floatAccessor[3] = 4.23;
  floatAccessor.write();

  auto rawAccessor = d.getOneDRegisterAccessor<int32_t>("FLOAT_TEST/ARRAY_AS_INT", 0, 0, {AccessMode::raw});
  rawAccessor.read();
  checkAsRaw(rawAccessor[0], 1.23);
  checkAsRaw(rawAccessor[1], 2.23);
  checkAsRaw(rawAccessor[2], 3.23);
  checkAsRaw(rawAccessor[3], 4.23);

  auto doubleAccessor = d.getOneDRegisterAccessor<double>("FLOAT_TEST/ARRAY");
  doubleAccessor[0] = 11.23;
  doubleAccessor[1] = 22.23;
  doubleAccessor[2] = 33.23;
  doubleAccessor[3] = 44.23;
  doubleAccessor.write();

  rawAccessor.read();
  checkAsRaw(rawAccessor[0], 11.23);
  checkAsRaw(rawAccessor[1], 22.23);
  checkAsRaw(rawAccessor[2], 33.23);
  checkAsRaw(rawAccessor[3], 44.23);

  auto intAccessor = d.getOneDRegisterAccessor<int>("FLOAT_TEST/ARRAY");
  intAccessor[0] = 1;
  intAccessor[1] = 2;
  intAccessor[2] = 3;
  intAccessor[3] = 4;
  intAccessor.write();

  rawAccessor.read();
  checkAsRaw(rawAccessor[0], 1.);
  checkAsRaw(rawAccessor[1], 2.);
  checkAsRaw(rawAccessor[2], 3.);
  checkAsRaw(rawAccessor[3], 4.);

  auto stringAccessor = d.getOneDRegisterAccessor<std::string>("FLOAT_TEST/ARRAY");
  stringAccessor[0] = "17.4";
  stringAccessor[1] = "17.5";
  stringAccessor[2] = "17.6";
  stringAccessor[3] = "17.7";
  stringAccessor.write();

  rawAccessor.read();
  checkAsRaw(rawAccessor[0], 17.4);
  checkAsRaw(rawAccessor[1], 17.5);
  checkAsRaw(rawAccessor[2], 17.6);
  checkAsRaw(rawAccessor[3], 17.7);
}
