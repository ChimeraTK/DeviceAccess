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

  // Use of overlapping ranges in transfer groups is undefined, so only use
  // the distinct accessors
  auto group = TransferGroup();
  group.addAccessor(accRangedLo);
  group.addAccessor(accRangedHi);

  accRangedHi = 0x75;
  accRangedLo = 0x80;

  group.write();
  accTarget.read();

  BOOST_TEST(accTarget == 0x7580);

  // Add overlapping accessor to group, check that the group cannot be written anymore
  group.addAccessor(accRangedMid);
  BOOST_CHECK_THROW(group.write(), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testAccessorSanity) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  // Manual test for spec B.2.4
  // Accessor too small for the configured number of bits
  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");

  auto accMiddle = device.getScalarRegisterAccessor<int8_t>("Middle");
  accTarget.setAndWrite(0x1fff);
  accMiddle.read();
  BOOST_TEST(accMiddle == 127);
  BOOST_CHECK(accMiddle.dataValidity() == ChimeraTK::DataValidity::faulty);

  // The Number of bits requested from the target register is larger than the register
  auto accTooLarge = device.getScalarRegisterAccessor<int16_t>("TooLarge");
  accTooLarge.setAndWrite(0xff1);
  accTarget.read();
  BOOST_CHECK(accTarget == std::numeric_limits<int16_t>::max());

  // The number of bits requested is smaller than what is available in the user type and the value
  // written in the accessor is larger than maximum value in those bits
  accTarget.setAndWrite(0);

  auto accMiddle2 = device.getScalarRegisterAccessor<int16_t>("MidByte");
  accMiddle2.setAndWrite(0x100);
  accTarget.read();
  BOOST_TEST(accTarget == 0x0ff0);
  BOOST_CHECK(accMiddle2.dataValidity() == ChimeraTK::DataValidity::faulty);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
