// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

///@todo FIXME My dynamic init header is a hack. Change the test to use
/// BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>

#include <math.h>

#include <algorithm>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

#define TEST_MAPPING_FILE "testDummyRegisterAccessors.map"
#define INVALID_MAPPING_FILE "invalidSequences.map"

// Test implementation of the dummy backend with two accessors
class DummyRegisterTest;
class TestableDummyBackend : public DummyBackend {
 public:
  explicit TestableDummyBackend(std::string mapFileName)
  : DummyBackend(mapFileName), someRegister(this, "APP0", "SOME_REGISTER"),
    someMuxedRegister(this, "APP0", "DAQ0_ADCA") {}

  DummyRegisterAccessor<int> someRegister;
  DummyMultiplexedRegisterAccessor<int> someMuxedRegister;

  friend class DummyRegisterTest;
};

// Test implementation of the dummy backend for the invalid map file
class InvalidDummyBackend : public DummyBackend {
 public:
  explicit InvalidDummyBackend(std::string mapFileName)
  : DummyBackend(mapFileName), invalidRegister(this, "INVALID", "NO_WORDS") {}

  DummyMultiplexedRegisterAccessor<int> invalidRegister;

  friend class DummyRegisterTest;
};

/**********************************************************************************************************************/
class DummyRegisterTest {
 public:
  DummyRegisterTest() { device = boost::shared_ptr<TestableDummyBackend>(new TestableDummyBackend(TEST_MAPPING_FILE)); }

  /// test exceptions
  void testExceptions();

  /// test the register accessor
  void testRegisterAccessor();

  /// test the register accessor
  void testMuxedRegisterAccessor();

 private:
  boost::shared_ptr<TestableDummyBackend> device;
  friend class DummyRegisterTestSuite;
};

/**********************************************************************************************************************/
class DummyRegisterTestSuite : public test_suite {
 public:
  DummyRegisterTestSuite() : test_suite("DummyRegister test suite") {
    boost::shared_ptr<DummyRegisterTest> dummyDeviceTest(new DummyRegisterTest);

    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testRegisterAccessor, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testMuxedRegisterAccessor, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testExceptions, dummyDeviceTest));
  }
};

/**********************************************************************************************************************/
bool init_unit_test() {
  framework::master_test_suite().p_name.value = "DummyRegister test suite";
  framework::master_test_suite().add(new DummyRegisterTestSuite);

  return true;
}

/**********************************************************************************************************************/
void DummyRegisterTest::testExceptions() {
  std::cout << "testExceptions" << std::endl;

  BOOST_CHECK_THROW(new InvalidDummyBackend(INVALID_MAPPING_FILE), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/
void DummyRegisterTest::testRegisterAccessor() {
  std::cout << "testRegisterAccessor" << std::endl;

  // open the device
  device->open();

  // check number of elements getter
  BOOST_CHECK(device->someRegister.getNumberOfElements() == 10);

  // test operator=
  device->someRegister = 3;
  BOOST_CHECK(device->_barContents[1][0] == 3);

  // test operator[] on r.h.s.
  device->_barContents[1][0] = 5;
  device->_barContents[1][3] = 77;
  BOOST_CHECK(device->someRegister[0] == 5);
  BOOST_CHECK(device->someRegister[3] == 77);

  // test operator[] on l.h.s.
  device->someRegister[0] = 666;
  device->someRegister[9] = 999;
  BOOST_CHECK(device->_barContents[1][0] == 666);
  BOOST_CHECK(device->_barContents[1][9] == 999);
  device->someRegister[1] = 111;
  device->someRegister[2] = 222;
  device->someRegister[3] = 333;
  device->someRegister[4] = 444;
  BOOST_CHECK(device->_barContents[1][1] == 111);
  BOOST_CHECK(device->_barContents[1][2] == 222);
  BOOST_CHECK(device->_barContents[1][3] == 333);
  BOOST_CHECK(device->_barContents[1][4] == 444);

  // test increment and decrement operators
  BOOST_CHECK(device->someRegister[1]++ == 111);
  BOOST_CHECK(device->someRegister[2]-- == 222);
  BOOST_CHECK(++device->someRegister[3] == 334);
  BOOST_CHECK(--device->someRegister[4] == 443);
  BOOST_CHECK(device->_barContents[1][1] == 112);
  BOOST_CHECK(device->_barContents[1][2] == 221);
  BOOST_CHECK(device->_barContents[1][3] == 334);
  BOOST_CHECK(device->_barContents[1][4] == 443);

  // close the device
  device->close();
}

/**********************************************************************************************************************/
void DummyRegisterTest::testMuxedRegisterAccessor() {
  std::cout << "testMuxedRegisterAccessor" << std::endl;

  // open the device
  device->open();

  // check number of elements getter
  BOOST_CHECK(device->someMuxedRegister.getNumberOfElements() == 4096);
  BOOST_CHECK(device->someMuxedRegister.getNumberOfSequences() == 16);

  // the area offset is 1000 bytes. When addressing the index of the 32 bit word
  // in the bar, we have to divide by 4
  static const int areaIndexOffset = 1000 / 4;

  // since our register does not have a fixed type, we use this union/struct to
  // fill the bar content directly the packed attribute prevents the compiler
  // from adding a padding between the struct fields
  union _mixedReg {
    struct _cooked {
      int32_t r0;
      int16_t r1;
      int16_t r2;
      int8_t r3;
      int8_t r4;
      int32_t r5;
      int16_t r6;
      int32_t r7;
      int32_t r8;
      int32_t r9;
      int32_t r10;
      int32_t r11;
      int32_t r12;
      int32_t r13;
      int32_t r14;
      uint32_t r15;
    } __attribute__((packed)) cooked;
    int32_t raw[13];
  } mixedReg;

  int pitch = sizeof(mixedReg.raw);

  // fill the register
  mixedReg.cooked.r0 = 42;
  mixedReg.cooked.r1 = 120;
  mixedReg.cooked.r2 = 222;
  mixedReg.cooked.r3 = -110;
  mixedReg.cooked.r4 = 1;
  mixedReg.cooked.r5 = 33;
  mixedReg.cooked.r6 = 6;
  mixedReg.cooked.r7 = 7;
  mixedReg.cooked.r8 = 8;
  mixedReg.cooked.r9 = 9;
  mixedReg.cooked.r10 = 10;
  mixedReg.cooked.r11 = 11;
  mixedReg.cooked.r12 = 12;
  mixedReg.cooked.r13 = 13;
  mixedReg.cooked.r14 = 14;
  mixedReg.cooked.r15 = 15;
  for(int i = 0; i < 13; i++) device->_barContents[0xD][areaIndexOffset + i] = mixedReg.raw[i];

  // test the test, to be sure the union is not going wrong
  BOOST_CHECK((int)((char*)&(mixedReg.cooked.r5) - (char*)&(mixedReg.cooked.r0)) == 10);
  BOOST_CHECK((int)((char*)&(mixedReg.cooked.r10) - (char*)&(mixedReg.cooked.r0)) == 28);
  BOOST_CHECK((int)((char*)&(mixedReg.cooked.r15) - (char*)&(mixedReg.cooked.r0)) == pitch - 4);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + 0] == 42);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + 1] == 120 + 0x10000 * 222);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + 9] == 12);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + 12] == 15);

  mixedReg.cooked.r0 = 1;
  mixedReg.cooked.r1 = 11;
  mixedReg.cooked.r2 = 22;
  mixedReg.cooked.r3 = 33;
  mixedReg.cooked.r4 = 0;
  mixedReg.cooked.r5 = 55;
  mixedReg.cooked.r6 = 66;
  mixedReg.cooked.r7 = 77;
  mixedReg.cooked.r8 = 88;
  mixedReg.cooked.r9 = 99;
  mixedReg.cooked.r10 = 100;
  mixedReg.cooked.r11 = 111;
  mixedReg.cooked.r12 = 222;
  mixedReg.cooked.r13 = 333;
  mixedReg.cooked.r14 = 444;
  mixedReg.cooked.r15 = 555;
  for(int i = 0; i < 13; i++) device->_barContents[0xD][areaIndexOffset + pitch / 4 + i] = mixedReg.raw[i];

  // test the test, to be sure the union is not going wrong
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + pitch / 4 + 0] == 1);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + pitch / 4 + 1] == 11 + 0x10000 * 22);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + pitch / 4 + 9] == 222);
  BOOST_CHECK(device->_barContents[0xD][areaIndexOffset + pitch / 4 + 12] == 555);

  // fill the rest of the register (has 4096 samples per channel)
  for(int i = 2; i < 4096; i++) {
    mixedReg.cooked.r0 = i + 0;
    mixedReg.cooked.r1 = i + 1;
    mixedReg.cooked.r2 = i + 2;
    mixedReg.cooked.r3 = i + 3;
    mixedReg.cooked.r4 = i + 4;
    mixedReg.cooked.r5 = i + 5;
    mixedReg.cooked.r6 = i + 6;
    mixedReg.cooked.r7 = i + 7;
    mixedReg.cooked.r8 = i + 8;
    mixedReg.cooked.r9 = i + 9;
    mixedReg.cooked.r10 = i + 10;
    mixedReg.cooked.r11 = i + 11;
    mixedReg.cooked.r12 = i + 12;
    mixedReg.cooked.r13 = i + 13;
    mixedReg.cooked.r14 = i + 14;
    mixedReg.cooked.r15 = i + 15;
    for(int k = 0; k < 13; k++) device->_barContents[0xD][areaIndexOffset + i * (pitch / 4) + k] = mixedReg.raw[k];
  }

  // test reading by [][] operator
  BOOST_CHECK(device->someMuxedRegister[0][0] == 42);
  BOOST_CHECK(device->someMuxedRegister[1][0] == 120);
  BOOST_CHECK(device->someMuxedRegister[2][0] == 222);
  BOOST_CHECK(device->someMuxedRegister[3][0] == -110);
  BOOST_CHECK(device->someMuxedRegister[4][0] == 1);
  BOOST_CHECK(device->someMuxedRegister[5][0] == 33);
  BOOST_CHECK(device->someMuxedRegister[6][0] == 6);
  BOOST_CHECK(device->someMuxedRegister[7][0] == 7);
  BOOST_CHECK(device->someMuxedRegister[8][0] == 8);
  BOOST_CHECK(device->someMuxedRegister[9][0] == 9);
  BOOST_CHECK(device->someMuxedRegister[10][0] == 10);
  BOOST_CHECK(device->someMuxedRegister[11][0] == 11);
  BOOST_CHECK(device->someMuxedRegister[12][0] == 12);
  BOOST_CHECK(device->someMuxedRegister[13][0] == 13);
  BOOST_CHECK(device->someMuxedRegister[14][0] == 14);
  BOOST_CHECK(device->someMuxedRegister[15][0] == 15);

  BOOST_CHECK(device->someMuxedRegister[0][1] == 1);
  BOOST_CHECK(device->someMuxedRegister[1][1] == 11);
  BOOST_CHECK(device->someMuxedRegister[2][1] == 22);
  BOOST_CHECK(device->someMuxedRegister[3][1] == 33);
  BOOST_CHECK(device->someMuxedRegister[4][1] == 0);
  BOOST_CHECK(device->someMuxedRegister[5][1] == 55);
  BOOST_CHECK(device->someMuxedRegister[6][1] == 66);
  BOOST_CHECK(device->someMuxedRegister[7][1] == 77);
  BOOST_CHECK(device->someMuxedRegister[8][1] == 88);
  BOOST_CHECK(device->someMuxedRegister[9][1] == 99);
  BOOST_CHECK(device->someMuxedRegister[10][1] == 100);
  BOOST_CHECK(device->someMuxedRegister[11][1] == 111);
  BOOST_CHECK(device->someMuxedRegister[12][1] == 222);
  BOOST_CHECK(device->someMuxedRegister[13][1] == 333);
  BOOST_CHECK(device->someMuxedRegister[14][1] == 444);
  BOOST_CHECK(device->someMuxedRegister[15][1] == 555);

  for(int i = 2; i < 65536 / 16; i++) {
    for(int k = 0; k < 16; k++) {
      int expectedValue = i + k;
      void* ptr = (void*)&expectedValue;
      if(k == 1 || k == 2 || k == 6) { // 16 bit
        expectedValue = *(reinterpret_cast<int16_t*>(ptr));
      }
      else if(k == 3) { // 8 bit
        expectedValue = *(reinterpret_cast<int8_t*>(ptr));
      }
      else if(k == 4) { // 1 bit
        expectedValue = expectedValue & 0x1;
      }
      else if(k == 7) { // 24 bit
        expectedValue = expectedValue & 0xFFFFFF;
      }
      std::stringstream message;
      message << "someMuxedRegister[" << k << "][" << i << "] == " << device->someMuxedRegister[k][i] << " but "
              << expectedValue << " expected.";
      BOOST_CHECK_MESSAGE(device->someMuxedRegister[k][i] == expectedValue, message.str());
    }
  }

  // test writing by [][] operator
  device->someMuxedRegister[0][0] = 666;
  device->someMuxedRegister[1][0] = 999;
  device->someMuxedRegister[2][0] = 222;
  device->someMuxedRegister[3][0] = -111;
  device->someMuxedRegister[4][0] = 0;
  device->someMuxedRegister[5][0] = 555;
  device->someMuxedRegister[6][0] = 666;
  device->someMuxedRegister[7][0] = 777;
  device->someMuxedRegister[8][0] = 888;
  device->someMuxedRegister[9][0] = 999;
  device->someMuxedRegister[10][0] = 1111;
  device->someMuxedRegister[11][0] = 2222;
  device->someMuxedRegister[12][0] = 3333;
  device->someMuxedRegister[13][0] = 4444;
  device->someMuxedRegister[14][0] = 5555;
  device->someMuxedRegister[15][0] = 6666;

  for(int i = 1; i < 65536 / 16; i++) {
    for(int k = 0; k < 16; k++) {
      device->someMuxedRegister[k][i] = 10 * k + i;
    }
  }

  for(int k = 0; k < 13; k++) mixedReg.raw[k] = device->_barContents[0xD][areaIndexOffset + k];
  BOOST_CHECK(mixedReg.cooked.r0 == 666);
  BOOST_CHECK(mixedReg.cooked.r1 == 999);
  BOOST_CHECK(mixedReg.cooked.r2 == 222);
  BOOST_CHECK(mixedReg.cooked.r3 == -111);
  BOOST_CHECK(mixedReg.cooked.r4 == 0);
  BOOST_CHECK(mixedReg.cooked.r5 == 555);
  BOOST_CHECK(mixedReg.cooked.r6 == 666);
  BOOST_CHECK(mixedReg.cooked.r7 == 777);
  BOOST_CHECK(mixedReg.cooked.r8 == 888);
  BOOST_CHECK(mixedReg.cooked.r9 == 999);
  BOOST_CHECK(mixedReg.cooked.r10 == 1111);
  BOOST_CHECK(mixedReg.cooked.r11 == 2222);
  BOOST_CHECK(mixedReg.cooked.r12 == 3333);
  BOOST_CHECK(mixedReg.cooked.r13 == 4444);
  BOOST_CHECK(mixedReg.cooked.r14 == 5555);
  BOOST_CHECK(mixedReg.cooked.r15 == 6666);

  for(int i = 1; i < 65536 / 16; i++) {
    for(int k = 0; k < 13; k++) mixedReg.raw[k] = device->_barContents[0xD][areaIndexOffset + i * (pitch / 4) + k];
    BOOST_CHECK(mixedReg.cooked.r0 == 10 * 0 + i);
    BOOST_CHECK(mixedReg.cooked.r1 == std::min(10 * 1 + i, 32767));
    BOOST_CHECK(mixedReg.cooked.r2 == std::min(10 * 2 + i, 32767));
    BOOST_CHECK(mixedReg.cooked.r3 == std::min(10 * 3 + i, 127));
    BOOST_CHECK(mixedReg.cooked.r4 == std::min(10 * 4 + i, 1));
    BOOST_CHECK(mixedReg.cooked.r5 == 10 * 5 + i);
    BOOST_CHECK(mixedReg.cooked.r6 == std::min(10 * 6 + i, 32767));
    BOOST_CHECK(mixedReg.cooked.r7 == 10 * 7 + i);
    BOOST_CHECK(mixedReg.cooked.r8 == 10 * 8 + i);
    BOOST_CHECK(mixedReg.cooked.r9 == 10 * 9 + i);
    BOOST_CHECK(mixedReg.cooked.r10 == 10 * 10 + i);
    BOOST_CHECK(mixedReg.cooked.r11 == 10 * 11 + i);
    BOOST_CHECK(mixedReg.cooked.r12 == 10 * 12 + i);
    BOOST_CHECK(mixedReg.cooked.r13 == 10 * 13 + i);
    BOOST_CHECK(mixedReg.cooked.r14 == 10 * 14 + i);
    BOOST_CHECK(mixedReg.cooked.r15 == (unsigned int)(10 * 15 + i));
  }

  // close the device
  device->close();
}
