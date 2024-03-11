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

/********************************************************************************************************************/

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
  // FIXME: This is currently not implemented in the plugin, because it needs changes in the fixed point converter
  // see https://redmine.msktools.desy.de/issues/12912
  // BOOST_CHECK(accMiddle2.dataValidity() == ChimeraTK::DataValidity::faulty);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMathPluginChaining) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  accTarget.setAndWrite(0x1fff);

  // Write some value in range (range is 0-5)
  auto accClamped = device.getScalarRegisterAccessor<int8_t>("LoByteClamped");
  accClamped.setAndWrite(0x01);
  accTarget.read();
  BOOST_TEST(accTarget == 0x1f01);
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);

  // Write some value outside of the clamped range
  accClamped.setAndWrite(55);
  accTarget.read();
  BOOST_TEST(accTarget == 0x1f05);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBitExtraction) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  accTarget.setAndWrite(0x5555);

  auto accRangedHi = device.getScalarRegisterAccessor<uint16_t>("HiByte");

  auto accBit0 = device.getScalarRegisterAccessor<ChimeraTK::Boolean>("Bit0");
  auto accBit1 = device.getScalarRegisterAccessor<ChimeraTK::Boolean>("Bit1");
  auto accBit2 = device.getScalarRegisterAccessor<ChimeraTK::Boolean>("Bit2");
  auto accBit3 = device.getScalarRegisterAccessor<ChimeraTK::Boolean>("Bit3");

  // See that the bits we get match the value we have
  accBit0.read();
  accBit1.read();
  accBit2.read();
  accBit3.read();

  BOOST_TEST(accBit0);
  BOOST_TEST(!accBit1);
  BOOST_TEST(accBit2);
  BOOST_TEST(!accBit3);

  // Write to the part that is not mapped to single bits
  // make sure the single bits are not modified
  accRangedHi.setAndWrite(0x11);
  accTarget.read();

  BOOST_TEST(accTarget == 0x1155);

  accBit0.read();
  accBit1.read();
  accBit2.read();
  accBit3.read();

  BOOST_TEST(accBit0);
  BOOST_TEST(!accBit1);
  BOOST_TEST(accBit2);
  BOOST_TEST(!accBit3);

  // Toggle single bits, make sure that this does not spread across the rest of the bits
  accBit1.setAndWrite(true);
  accBit3.setAndWrite(true);

  accTarget.read();

  BOOST_TEST(accTarget == 0x115F);
  accRangedHi.read();
  BOOST_TEST(accRangedHi == 0x11);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataDescription) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=bitRangeReadPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  accTarget.setAndWrite(0x5555);

  auto accLo = device.getScalarRegisterAccessor<uint8_t>("LoByte");
  auto accLoSigned = device.getScalarRegisterAccessor<int8_t>("LowerSigned");

  accLo.read();
  accLoSigned.read();
  BOOST_TEST(accLo == 85);
  BOOST_TEST(accLoSigned == 85);

  accTarget.setAndWrite(0x5580);

  accLo.read();
  accLoSigned.read();

  BOOST_TEST(int(accLo) == 128);
  BOOST_TEST(int(accLoSigned) == -128);

  accTarget.setAndWrite(0x5555);
  auto accFixed = device.getScalarRegisterAccessor<float>("LowerFixedPoint");
  accFixed.read();
  BOOST_TEST(accFixed == 5.3125);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
