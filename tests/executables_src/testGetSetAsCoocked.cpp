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

  Device d("JDEV");
  d.open();

  // APP/STATUS/ErrorCounter is a bitField register sub-element with bitShift 2, width 3, unsigned.
  // With raw access, buffer_2D stores the full 32-bit register word.
  // auto errorCounter = d.getScalarRegisterAccessor<uint32_t>("APP/STATUS/ErrorCounter", 0, {AccessMode::raw});
  auto errorCounter = d.getScalarRegisterAccessor<uint32_t>("APP/STATUS/ErrorCounter", 0, {AccessMode::raw});

  // APP/STATUS is the full 32 bit word behind ErrorCounter. The content should be identical to the raw access
  auto status = d.getScalarRegisterAccessor<uint32_t>("APP/STATUS/DUMMY_WRITEABLE");

  status.setAndWrite(0b110110); // Also bits 1 and 5 are set, but not part of ErrorCounter

  errorCounter.read();
  // Test: The complete raw word as it exists on the device is available as raw data. Reading worked.
  BOOST_TEST(errorCounter == 0b110110);
  // Test: getAsCooked() is extracting the correct bits [2:4]
  BOOST_TEST(errorCounter.getAsCooked<uint32_t>() == 5);

  // Test: setAsCooked on ErrorCounter should only affect bits [4:2], not the other bits
  errorCounter.setAsCooked(2); // ErrorCounter=2 (binary 010)
  BOOST_TEST(errorCounter == 0b101010);

  status.setAndWrite(0);

  // Writing writes the whole word.
  errorCounter.write();
  BOOST_TEST(status.readAndGet() == 0b101010);
  BOOST_TEST(errorCounter.isWriteable());
}
