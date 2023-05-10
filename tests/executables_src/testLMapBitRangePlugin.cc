// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TransferGroup.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapBitRangePluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapBitRangeTestSuite)

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testSimpleRead) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");

  auto accRangedHi = device.getScalarRegisterAccessor<uint16_t>("HiByte");
  auto accRangedMid = device.getScalarRegisterAccessor<uint16_t>("MidByte");
  auto accRangedLo = device.getScalarRegisterAccessor<uint16_t>("LoByte");

  accTarget.setAndWrite(0x1f0f);

  accRangedLo.read();
  accRangedHi.read();
  accRangedMid.read();

  BOOST_TEST(accRangedLo == 0x0f);
  BOOST_TEST(accRangedHi == 0x1f);
  BOOST_TEST(accRangedMid == 0xf0);


  TransferGroup group;
  group.addAccessor(accRangedLo);
  group.addAccessor(accRangedHi);

  accTarget.setAndWrite(0);
  group.read();
  BOOST_TEST(accRangedLo == 0);
  BOOST_TEST(accRangedHi == 0);

  accTarget.setAndWrite(0x5a1f);
  group.read();
  BOOST_TEST(accRangedLo == 0x1f);
  BOOST_TEST(accRangedHi == 0x5a);
}

BOOST_AUTO_TEST_CASE(testSimpleWrite) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");

  auto accRangedHi = device.getScalarRegisterAccessor<uint16_t>("HiByte");
  auto accRangedMid = device.getScalarRegisterAccessor<uint16_t>("MidByte");
  auto accRangedLo = device.getScalarRegisterAccessor<uint16_t>("LoByte");

  accTarget.setAndWrite(0x1f0f);
  accRangedHi = 0x76;
  accRangedHi.write();

  accRangedMid.read();
  BOOST_TEST(accRangedMid == 0x60);
  accTarget.read();
  BOOST_TEST(accTarget == 0x760f);

  // Use of overlapping ranges is undefined
  auto group = TransferGroup();
  group.addAccessor(accRangedLo);
  group.addAccessor(accRangedHi);

  accRangedHi = 0x75;
  accRangedLo = 0x80;

  group.dump();
  group.write();
  accTarget.read();

  BOOST_TEST(accTarget == 0x7580);
}

BOOST_AUTO_TEST_CASE(testAccessorSanity) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  // Accessor too small for the configured number of bits
  BOOST_CHECK_THROW(std::ignore = device.getScalarRegisterAccessor<uint8_t>("Middle"), ChimeraTK::logic_error);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
