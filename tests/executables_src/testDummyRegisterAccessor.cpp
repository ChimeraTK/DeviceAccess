// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

///@todo FIXME My dynamic init header is a hack. Change the test to use
/// BOOST_AUTO_TEST_CASE!
#include "BackendFactory.h"
#include "boost_dynamic_init_test.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>

#include <math.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <type_traits>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

#define TEST_MAPPING_FILE "testDummyRegisterAccessors.map"
#define INVALID_MAPPING_FILE "invalidSequences.map"

// The chunk size is part of the public interface of DummyBackendBase, so the tests can build addresses relative to it
// without knowing its concrete value.
static constexpr uint64_t CHUNK = DummyBackendBase::CHUNK_SIZE;

/// Write a minimal jmap file with registers at odd byte offsets, a 16-bit register and a register spanning a chunk
/// boundary, then return its path.
static std::string writeChunkedBackdoorJmap() {
  std::string fileName = "chunkedBackdoorAccessor_" + std::to_string(getpid()) + ".jmap";
  std::ofstream file(fileName);
  file << R"({
  "mapFormatVersion": "1.0",
  "metadata": {},
  "interruptHandler": {},
  "addressSpace": {
    "MODULE.MISALIGNED": {
      "bytesPerElement": 4,
      "numberOfElements": 3,
      "address": {
        "type": "IO",
        "channel": 1,
        "offset": 3
      },
      "representation": {
        "isSigned": false
      }
    },
    "MODULE.SIXTEEN_BIT": {
      "bytesPerElement": 2,
      "numberOfElements": 4,
      "address": {
        "type": "IO",
        "channel": 1,
        "offset": 32
      },
      "representation": {
        "isSigned": false
      }
    },
    "MODULE.BOUNDARY": {
      "bytesPerElement": 4,
      "numberOfElements": 4,
      "address": {
        "type": "IO",
        "channel": 2,
        "offset": )"
       << (CHUNK - 2 * sizeof(int32_t)) << R"(
      },
      "representation": {
        "isSigned": false
      }
    }
  }
})";
  file.close();
  return fileName;
}

namespace {

  /// DummyBackend exposing the chunked backdoor accessors for the temporary map file, which lives at odd byte offsets
  /// and across a chunk boundary.
  class ChunkedBackdoorDummyBackend : public DummyBackend {
   public:
    explicit ChunkedBackdoorDummyBackend(const std::string& mapFileName)
    : DummyBackend(mapFileName), misaligned(this, "MODULE", "MISALIGNED"), boundary(this, "MODULE", "BOUNDARY") {}

    DummyRegisterAccessor<int> misaligned;
    DummyRegisterAccessor<int> boundary;
  };

} // namespace

// Test implementation of the dummy backend with two accessors
class DummyRegisterTest;
class TestableDummyBackend : public DummyBackend {
 public:
  explicit TestableDummyBackend(std::string mapFileName)
  : DummyBackend(mapFileName), someRegister(this, "APP0", "SOME_REGISTER"),
    someMuxedRegister(this, "APP0", "DAQ0_ADCA") {}

  DummyRegisterAccessor<int> someRegister;
  DummyMultiplexedRegisterAccessor<int> someMuxedRegister;

  /// Backdoor accessor used to place the pre-packed 32-bit words into the memory; the internal bar contents are
  /// chunked byte storage and are not written directly any more. It addresses the multiplexed area through its 1D raw
  /// view ("APP0.DAQ0_ADCA.MULTIPLEXED_RAW"), so that consecutive indices map to consecutive transport words from the
  /// area start, exactly like the pre-change word indexing into the bar contents. The raw accessor is created through
  /// DummyBackend::getRawAccessor(), which supplies the shared pointer to the backend.
  std::unique_ptr<DummyRegisterRawAccessor<std::int32_t>> muxedBackdoor;

  // NOLINTNEXTLINE(performance-unnecessary-value-param)
  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new TestableDummyBackend(parameters["map"]));
  }
  class BackendRegisterer {
   public:
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "TestableDummy", &TestableDummyBackend::createInstance);
    }
  };
  static BackendRegisterer backendRegisterer;
};
TestableDummyBackend::BackendRegisterer TestableDummyBackend::backendRegisterer;

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
  DummyRegisterTest() {
    device = boost::dynamic_pointer_cast<TestableDummyBackend>(
        BackendFactory::getInstance().createBackend("(TestableDummy?map=" TEST_MAPPING_FILE ")"));
    device->muxedBackdoor = std::make_unique<DummyRegisterRawAccessor<std::int32_t>>(
        device->getRawAccessor("APP0", "DAQ0_ADCA.MULTIPLEXED_RAW"));
  }

  /// test exceptions
  void testExceptions();

  /// test the register accessor
  void testRegisterAccessor();

  /// test the register accessor
  void testMuxedRegisterAccessor();

  /// test a backdoor accessor at an odd byte offset, and the byte-granular semantics enabled by chunked storage
  void testChunkedBackdoorByteAlignedRegister();

  /// test a backdoor accessor for a register spanning a chunk boundary
  void testChunkedBackdoorRegisterSpanningChunkBoundary();

  /// test the value semantics of the chunked raw accessor
  void testChunkedBackdoorRawAccessorValueSemantics();

  /// test the chunked raw accessor instantiated for a non-default RawType
  void testChunkedBackdoorRawAccessorNonDefaultRawType();

  /// test the deduction guide of the chunked raw accessor
  void testChunkedBackdoorRawAccessorDeductionGuide();

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
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testChunkedBackdoorByteAlignedRegister, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testChunkedBackdoorRegisterSpanningChunkBoundary, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testChunkedBackdoorRawAccessorValueSemantics, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testChunkedBackdoorRawAccessorNonDefaultRawType, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyRegisterTest::testChunkedBackdoorRawAccessorDeductionGuide, dummyDeviceTest));
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

  // This test exercises the public API (ordinary accessors), not the backdoor accessor, so it covers the accessor
  // layer rather than the backdoor accessor against itself.
  auto accessor = device->getRegisterAccessor<int>("APP0.SOME_REGISTER", 10, 0, {});

  // check number of elements getter
  BOOST_CHECK(accessor->getNumberOfSamples() == 10);

  // write a full block through the ordinary one-dimensional API and read it back
  std::vector<int32_t> data{0, 11, 22, 33, 44, 55, 66, 77, 88, 99};
  accessor->accessChannel(0) = data;
  accessor->write();

  accessor->read();
  BOOST_CHECK(accessor->accessChannel(0) == data);

  // write a single element and read it back
  accessor->accessChannel(0)[3] = 77;
  accessor->write();
  accessor->read();
  BOOST_CHECK(accessor->accessChannel(0)[3] == 77);

  // elements that were never touched still read back their previously written value
  BOOST_CHECK(accessor->accessChannel(0)[0] == 0);
  BOOST_CHECK(accessor->accessChannel(0)[9] == 99);

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

  // the area offset is 1000 bytes into the bar; the accessor addresses it by byte offset directly

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
  for(int i = 0; i < 13; i++) {
    int32_t word;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) deliberate union member access
    std::memcpy(&word, &mixedReg.raw[i], sizeof(word));
    (*device->muxedBackdoor)[i] = word;
  }

  // test the test, to be sure the union is not going wrong
  BOOST_CHECK((int)((char*)&(mixedReg.cooked.r5) - (char*)&(mixedReg.cooked.r0)) == 10);
  BOOST_CHECK((int)((char*)&(mixedReg.cooked.r10) - (char*)&(mixedReg.cooked.r0)) == 28);
  BOOST_CHECK((int)((char*)&(mixedReg.cooked.r15) - (char*)&(mixedReg.cooked.r0)) == pitch - 4);

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
  for(int i = 0; i < 13; i++) {
    int32_t word;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) deliberate union member access
    std::memcpy(&word, &mixedReg.raw[i], sizeof(word));
    (*device->muxedBackdoor)[13 + i] = word;
  }

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
    for(int k = 0; k < 13; k++) {
      int32_t word;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) deliberate union member access
      std::memcpy(&word, &mixedReg.raw[k], sizeof(word));
      (*device->muxedBackdoor)[i * 13 + k] = word;
    }
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

  for(int k = 0; k < 13; k++) {
    int32_t word = (*device->muxedBackdoor)[k];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) deliberate union member access
    std::memcpy(&mixedReg.raw[k], &word, sizeof(word));
  }
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
    for(int k = 0; k < 13; k++) {
      int32_t word = (*device->muxedBackdoor)[i * 13 + k];
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) deliberate union member access
      std::memcpy(&mixedReg.raw[k], &word, sizeof(word));
    }
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

/**********************************************************************************************************************/
void DummyRegisterTest::testChunkedBackdoorByteAlignedRegister() {
  std::cout << "testChunkedBackdoorByteAlignedRegister" << std::endl;

  std::string mapFile = writeChunkedBackdoorJmap();
  auto backend = boost::make_shared<ChunkedBackdoorDummyBackend>(mapFile);
  backend->open();

  // A full word just after the register (which occupies bytes 3..14) stays untouched by the byte-granular accesses.
  const int32_t neighbourValue = 0x0a0b0c0d;
  const uint64_t bar = 1;
  const uint64_t neighbourAddress = 16;
  backend->write(bar, neighbourAddress, &neighbourValue, sizeof(int32_t));

  // Write through the accessor: elements are 4 bytes wide starting at byte offset 3.
  backend->misaligned[0] = 0x11111111;
  backend->misaligned[1] = 0x22222222;
  backend->misaligned[2] = 0x33333333;
  BOOST_CHECK((int)backend->misaligned[0] == 0x11111111);
  BOOST_CHECK((int)backend->misaligned[1] == 0x22222222);
  BOOST_CHECK((int)backend->misaligned[2] == 0x33333333);

  // The bytes after the register were not modified by the byte-granular element accesses.
  int32_t neighbour = 0;
  backend->read(bar, neighbourAddress, &neighbour, sizeof(int32_t));
  BOOST_CHECK(neighbour == neighbourValue);

  // The first register byte sits at offset 3, so the low byte of a word at offset 0 (bytes 0..2) is zero.
  int32_t word = -1;
  const uint64_t firstAddress = 0;
  backend->read(bar, firstAddress, &word, sizeof(int32_t));
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  BOOST_CHECK((word & 0x00ffffff) == 0);

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/
void DummyRegisterTest::testChunkedBackdoorRegisterSpanningChunkBoundary() {
  std::cout << "testChunkedBackdoorRegisterSpanningChunkBoundary" << std::endl;

  std::string mapFile = writeChunkedBackdoorJmap();
  auto backend = boost::make_shared<ChunkedBackdoorDummyBackend>(mapFile);
  backend->open();

  // The register starts two words before the chunk boundary and has 4 elements, so it straddles it.
  backend->boundary[0] = 0x0a0a0a0a;
  backend->boundary[1] = 0x0b0b0b0b;
  backend->boundary[2] = 0x0c0c0c0c;
  backend->boundary[3] = 0x0d0d0d0d;

  BOOST_CHECK((int)backend->boundary[0] == 0x0a0a0a0a);
  BOOST_CHECK((int)backend->boundary[1] == 0x0b0b0b0b);
  BOOST_CHECK((int)backend->boundary[2] == 0x0c0c0c0c);
  BOOST_CHECK((int)backend->boundary[3] == 0x0d0d0d0d);

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/
void DummyRegisterTest::testChunkedBackdoorRawAccessorValueSemantics() {
  std::cout << "testChunkedBackdoorRawAccessorValueSemantics" << std::endl;

  std::string mapFile = writeChunkedBackdoorJmap();
  auto backend = boost::make_shared<ChunkedBackdoorDummyBackend>(mapFile);
  backend->open();

  auto raw = backend->getRawAccessor("MODULE", "MISALIGNED");

  raw = 100;
  BOOST_CHECK(int(raw) == 100);

  raw += 5;
  BOOST_CHECK(int(raw) == 105);
  raw -= 3;
  BOOST_CHECK(int(raw) == 102);
  raw *= 2;
  BOOST_CHECK(int(raw) == 204);
  raw /= 4;
  BOOST_CHECK(int(raw) == 51);
  raw %= 10;
  BOOST_CHECK(int(raw) == 1);

  raw = 0x0f0f;
  raw &= 0x00ff;
  BOOST_CHECK(int(raw) == 0x000f);
  raw |= 0x00f0;
  BOOST_CHECK(int(raw) == 0x00ff);
  raw ^= 0x000f;
  BOOST_CHECK(int(raw) == 0x00f0);
  raw <<= 4;
  BOOST_CHECK(int(raw) == 0x0f00);
  raw >>= 4;
  BOOST_CHECK(int(raw) == 0x00f0);

  raw = 10;
  BOOST_CHECK(++raw == 11);
  BOOST_CHECK(--raw == 10);
  BOOST_CHECK(raw++ == 10);
  BOOST_CHECK(int(raw) == 11);
  BOOST_CHECK(raw-- == 11);
  BOOST_CHECK(int(raw) == 10);

  raw = 0x1234;
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  BOOST_CHECK((raw & 0x00ff) == 0x0034);
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  BOOST_CHECK((~raw) == ~0x1234);

  // The memory itself holds the last written value.
  int32_t word = 0;
  const uint64_t bar = 1;
  const uint64_t address = 3;
  backend->read(bar, address, &word, sizeof(int32_t));
  BOOST_CHECK(word == 0x1234);

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/
void DummyRegisterTest::testChunkedBackdoorRawAccessorNonDefaultRawType() {
  std::cout << "testChunkedBackdoorRawAccessorNonDefaultRawType" << std::endl;

  std::string mapFile = writeChunkedBackdoorJmap();
  auto backend = boost::make_shared<ChunkedBackdoorDummyBackend>(mapFile);
  backend->open();

  // A non-default raw type is obtained through the templated factory, like the default through a deduction-less call.
  auto raw = backend->getRawAccessor<int16_t>("MODULE", "SIXTEEN_BIT");

  BOOST_CHECK(raw.getNumberOfElements() == 4U);

  raw[0] = int16_t(0x1234);
  raw[1] = int16_t(0x5678);
  raw[2] = int16_t(-1);
  raw[3] = int16_t(-32768);

  BOOST_CHECK(raw[0].get() == int16_t(0x1234));
  BOOST_CHECK(raw[1].get() == int16_t(0x5678));
  BOOST_CHECK(raw[2].get() == int16_t(-1));
  BOOST_CHECK(raw[3].get() == int16_t(-32768));

  // Value semantics with the narrower raw type.
  raw[0] += int16_t(1);
  BOOST_CHECK(raw[0].get() == int16_t(0x1235));

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/
void DummyRegisterTest::testChunkedBackdoorRawAccessorDeductionGuide() {
  std::cout << "testChunkedBackdoorRawAccessorDeductionGuide" << std::endl;

  std::string mapFile = writeChunkedBackdoorJmap();
  auto backend = boost::make_shared<ChunkedBackdoorDummyBackend>(mapFile);
  backend->open();

  // No explicit template argument: the deduction guide fixes RawType to int32_t. This only compiles if the guide
  // resolves to a 32-bit type, because the 4-byte register would fail the element-width assert otherwise.
  DummyRegisterRawAccessor deduced(backend, "MODULE", "MISALIGNED");
  static_assert(std::is_same_v<decltype(deduced), DummyRegisterRawAccessor<int32_t>>);

  deduced[0] = 0x0badf00d;
  BOOST_CHECK(int(deduced[0]) == 0x0badf00d);

  backend->close();
  std::remove(mapFile.c_str());
}
