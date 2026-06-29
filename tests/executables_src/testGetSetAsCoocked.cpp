// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE GetSetAsCookedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <Device.h>
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE(testRawAccessor) {
  setDMapFilePath("dummies.dmap");

  Device d;
  d.open("DUMMYD3");

  auto scalarRawAccessor = d.getScalarRegisterAccessor<int32_t>("BOARD/WORD_USER", 0, {AccessMode::raw});
  scalarRawAccessor = 25;
  // the register has 3 fractional bits
  BOOST_CHECK(std::fabs(scalarRawAccessor.getAsCooked<double>() - 25. / 8) < 0.0001);

  scalarRawAccessor.setAsCooked(31. / 8);
  BOOST_CHECK_EQUAL(int32_t(scalarRawAccessor), 31);

  auto oneDRawAccessor = d.getOneDRegisterAccessor<int32_t>("ADC/AREA_DMAABLE_FIXEDPOINT16_3", 0, 0, {AccessMode::raw});

  oneDRawAccessor[0] = 12;
  oneDRawAccessor[1] = 13;

  // the register has 3 fractional bits
  BOOST_CHECK(std::fabs(oneDRawAccessor.getAsCooked<double>(0) - 12. / 8) < 0.0001);
  BOOST_CHECK(std::fabs(oneDRawAccessor.getAsCooked<double>(1) - 13. / 8) < 0.0001);

  oneDRawAccessor.setAsCooked(0, 42. / 8);
  oneDRawAccessor.setAsCooked(1, 43. / 8);

  BOOST_CHECK_EQUAL(oneDRawAccessor[0], 42);
  BOOST_CHECK_EQUAL(oneDRawAccessor[1], 43);
}

BOOST_AUTO_TEST_CASE(testRawAccessorBitField) {
  setDMapFilePath("simpleJsonFile.dmap");

  Device d;
  d.open("JDEV");

  // APP/STATUS is a bitField register: sub-element ProbeLimiter at bitShift 0, width 1 (unsigned).
  // With raw access, buffer_2D stores the full 32-bit register word.
  auto rawAccessor = d.getScalarRegisterAccessor<int32_t>("APP/STATUS", 0, {AccessMode::raw});
  rawAccessor.read();

  // The sharedMemoryDummy initializes to 0, so the full word is 0 and the bit-field is also 0.
  BOOST_CHECK_EQUAL(int32_t(rawAccessor), 0);
  BOOST_CHECK_EQUAL(rawAccessor.getAsCooked<double>(), 0.0);

  // Set raw value to a known bit pattern and verify getAsCooked extracts the correct bit field.
  // ProbeLimiter is bit 0 (shift 0, width 1). Set bit 0 to 1 -> raw value = 1, cooked = 1.0.
  rawAccessor = 1;
  BOOST_CHECK_EQUAL(rawAccessor.getAsCooked<double>(), 1.0);

  // setAsCooked should convert and merge into the full register word.
  rawAccessor.setAsCooked(0.0);
  BOOST_CHECK_EQUAL(int32_t(rawAccessor), 0);
}
