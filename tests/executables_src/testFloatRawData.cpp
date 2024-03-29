// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE FloatRawDataTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

/* This test is checking that IEE754 encoded single precision floats (32 bits) are transferred
 * corretly to and from int32_t raw registers.
 */

#include "Device.h"
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE(testCatalogueEntries) {
  Device d;
  d.open("(dummy?map=floatRawTest.map)");

  auto registerCatalogue = d.getRegisterCatalogue();
  auto scalarInfo = registerCatalogue.getRegister("FLOAT_TEST/SCALAR");

  BOOST_CHECK_EQUAL(scalarInfo.getRegisterName(), "FLOAT_TEST/SCALAR");
  BOOST_CHECK_EQUAL(scalarInfo.getNumberOfElements(), 1);
  BOOST_CHECK_EQUAL(scalarInfo.getNumberOfChannels(), 1);
  BOOST_CHECK_EQUAL(scalarInfo.getNumberOfDimensions(), 0);
  BOOST_CHECK(scalarInfo.isReadable());
  BOOST_CHECK(scalarInfo.isWriteable());

  BOOST_CHECK(scalarInfo.getSupportedAccessModes() == AccessModeFlags({AccessMode::raw}));

  auto dataDescriptor = scalarInfo.getDataDescriptor();
  BOOST_CHECK(dataDescriptor.fundamentalType() == DataDescriptor::FundamentalType::numeric);
  BOOST_CHECK(dataDescriptor.isSigned());
  BOOST_CHECK(dataDescriptor.isIntegral() == false);
  BOOST_CHECK_EQUAL(dataDescriptor.nDigits(), 48);
  BOOST_CHECK_EQUAL(dataDescriptor.nFractionalDigits(), 45);
  BOOST_CHECK_EQUAL(dataDescriptor.rawDataType(), DataType::int32);
  // FIXME: the following should be int32, but this layer is not accessible through the interface anyway.
  BOOST_CHECK_EQUAL(dataDescriptor.transportLayerDataType(), DataType::none);
}

BOOST_AUTO_TEST_CASE(testReading) {
  Device d;
  d.open("(dummy?map=floatRawTest.map)");

  // There are two ways to check what is going on in the dummy (we want to go back there and check that is ends up
  // correctly)
  // 1. We get the dummy backend and use DummyRegisterAccessors
  // 2. We use "integer" accessors pointing to the same memory, which have already been tested and we know that they
  // work Here we use the second approach.
  auto rawIntAccessor = d.getScalarRegisterAccessor<int32_t>("FLOAT_TEST/SCALAR_AS_INT", 0, {AccessMode::raw});
  rawIntAccessor = 0x40700000; // IEE754 bit representation of 3.75
  rawIntAccessor.write();

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

  auto rawAccessor = d.getScalarRegisterAccessor<int32_t>("FLOAT_TEST/SCALAR", 0, {AccessMode::raw});
  rawAccessor.read();
  BOOST_CHECK_EQUAL(int32_t(rawAccessor), 0x40700000);
  BOOST_CHECK_CLOSE(rawAccessor.getAsCooked<float>(), 3.75, 0.0001);
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

  auto rawIntAccessor = d.getOneDRegisterAccessor<int32_t>("FLOAT_TEST/ARRAY_AS_INT", 0, 0, {AccessMode::raw});
  rawIntAccessor.read();
  checkAsRaw(rawIntAccessor[0], 1.23);
  checkAsRaw(rawIntAccessor[1], 2.23);
  checkAsRaw(rawIntAccessor[2], 3.23);
  checkAsRaw(rawIntAccessor[3], 4.23);

  auto doubleAccessor = d.getOneDRegisterAccessor<double>("FLOAT_TEST/ARRAY");
  doubleAccessor[0] = 11.23;
  doubleAccessor[1] = 22.23;
  doubleAccessor[2] = 33.23;
  doubleAccessor[3] = 44.23;
  doubleAccessor.write();

  rawIntAccessor.read();
  checkAsRaw(rawIntAccessor[0], 11.23);
  checkAsRaw(rawIntAccessor[1], 22.23);
  checkAsRaw(rawIntAccessor[2], 33.23);
  checkAsRaw(rawIntAccessor[3], 44.23);

  auto intAccessor = d.getOneDRegisterAccessor<int>("FLOAT_TEST/ARRAY");
  intAccessor[0] = 1;
  intAccessor[1] = 2;
  intAccessor[2] = 3;
  intAccessor[3] = 4;
  intAccessor.write();

  rawIntAccessor.read();
  checkAsRaw(rawIntAccessor[0], 1.);
  checkAsRaw(rawIntAccessor[1], 2.);
  checkAsRaw(rawIntAccessor[2], 3.);
  checkAsRaw(rawIntAccessor[3], 4.);

  auto stringAccessor = d.getOneDRegisterAccessor<std::string>("FLOAT_TEST/ARRAY");
  stringAccessor[0] = "17.4";
  stringAccessor[1] = "17.5";
  stringAccessor[2] = "17.6";
  stringAccessor[3] = "17.7";
  stringAccessor.write();

  rawIntAccessor.read();
  checkAsRaw(rawIntAccessor[0], 17.4);
  checkAsRaw(rawIntAccessor[1], 17.5);
  checkAsRaw(rawIntAccessor[2], 17.6);
  checkAsRaw(rawIntAccessor[3], 17.7);
}
