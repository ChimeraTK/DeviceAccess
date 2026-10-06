// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ChunkedBackdoorAccessorTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

#include <boost/make_shared.hpp>

#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <type_traits>

using namespace ChimeraTK;

// The chunk size is part of the public interface of DummyBackendBase, so the tests can build addresses relative to it
// without knowing its concrete value.
static constexpr uint64_t CHUNK = DummyBackendBase::CHUNK_SIZE;

/// Write a minimal jmap file with registers at odd byte offsets, a 16-bit register and a register spanning a chunk
/// boundary, then return its path.
static std::string writeJmap() {
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

  /// DummyBackend exposing the affected backdoor accessors for a temporary map file.
  class TestableDummyBackend : public DummyBackend {
   public:
    explicit TestableDummyBackend(const std::string& mapFileName)
    : DummyBackend(mapFileName), misaligned(this, "MODULE", "MISALIGNED"), boundary(this, "MODULE", "BOUNDARY") {}

    DummyRegisterAccessor<int> misaligned;
    DummyRegisterAccessor<int> boundary;
  };

} // namespace

BOOST_AUTO_TEST_SUITE(ChunkedBackdoorAccessorTestSuite)

/**********************************************************************************************************************/

// A register at an odd byte offset (misaligned) is accessed correctly, and neighbouring bytes are not disturbed.
BOOST_AUTO_TEST_CASE(testByteAlignedRegister) {
  std::string mapFile = writeJmap();
  auto backend = boost::make_shared<TestableDummyBackend>(mapFile);
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
  BOOST_TEST((int)backend->misaligned[0] == 0x11111111);
  BOOST_TEST((int)backend->misaligned[1] == 0x22222222);
  BOOST_TEST((int)backend->misaligned[2] == 0x33333333);

  // The bytes after the register were not modified by the byte-granular element accesses.
  int32_t neighbour = 0;
  backend->read(bar, neighbourAddress, &neighbour, sizeof(int32_t));
  BOOST_TEST(neighbour == neighbourValue);

  // The first register byte sits at offset 3, so the low byte of a word at offset 0 (bytes 0..2) is zero.
  int32_t word = -1;
  const uint64_t firstAddress = 0;
  backend->read(bar, firstAddress, &word, sizeof(int32_t));
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  BOOST_TEST((word & 0x00ffffff) == 0);

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// A register spanning a chunk boundary is accessed correctly through the backdoor accessor.
BOOST_AUTO_TEST_CASE(testRegisterSpanningChunkBoundary) {
  std::string mapFile = writeJmap();
  auto backend = boost::make_shared<TestableDummyBackend>(mapFile);
  backend->open();

  // The register starts two words before the chunk boundary and has 4 elements, so it straddles it.
  backend->boundary[0] = 0x0a0a0a0a;
  backend->boundary[1] = 0x0b0b0b0b;
  backend->boundary[2] = 0x0c0c0c0c;
  backend->boundary[3] = 0x0d0d0d0d;

  BOOST_TEST((int)backend->boundary[0] == 0x0a0a0a0a);
  BOOST_TEST((int)backend->boundary[1] == 0x0b0b0b0b);
  BOOST_TEST((int)backend->boundary[2] == 0x0c0c0c0c);
  BOOST_TEST((int)backend->boundary[3] == 0x0d0d0d0d);

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// The raw accessor uses value semantics: the compound-assignment operators, inc/dec, the binary operators & and ~ and
// the conversion to the underlying type all work and update the memory.
BOOST_AUTO_TEST_CASE(testRawAccessorValueSemantics) {
  std::string mapFile = writeJmap();
  auto backend = boost::make_shared<TestableDummyBackend>(mapFile);
  backend->open();

  auto raw = backend->getRawAccessor("MODULE", "MISALIGNED");

  raw = 100;
  BOOST_TEST(int(raw) == 100);

  raw += 5;
  BOOST_TEST(int(raw) == 105);
  raw -= 3;
  BOOST_TEST(int(raw) == 102);
  raw *= 2;
  BOOST_TEST(int(raw) == 204);
  raw /= 4;
  BOOST_TEST(int(raw) == 51);
  raw %= 10;
  BOOST_TEST(int(raw) == 1);

  raw = 0x0f0f;
  raw &= 0x00ff;
  BOOST_TEST(int(raw) == 0x000f);
  raw |= 0x00f0;
  BOOST_TEST(int(raw) == 0x00ff);
  raw ^= 0x000f;
  BOOST_TEST(int(raw) == 0x00f0);
  raw <<= 4;
  BOOST_TEST(int(raw) == 0x0f00);
  raw >>= 4;
  BOOST_TEST(int(raw) == 0x00f0);

  raw = 10;
  BOOST_TEST(++raw == 11);
  BOOST_TEST(--raw == 10);
  BOOST_TEST(raw++ == 10);
  BOOST_TEST(int(raw) == 11);
  BOOST_TEST(raw-- == 11);
  BOOST_TEST(int(raw) == 10);

  raw = 0x1234;
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  BOOST_TEST((raw & 0x00ff) == 0x0034);
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  BOOST_TEST((~raw) == ~0x1234);

  // The memory itself holds the last written value.
  int32_t word = 0;
  const uint64_t bar = 1;
  const uint64_t address = 3;
  backend->read(bar, address, &word, sizeof(int32_t));
  BOOST_TEST(word == 0x1234);

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// The raw accessor instantiated for a non-default RawType (int16_t) accesses the memory correctly.
BOOST_AUTO_TEST_CASE(testRawAccessorNonDefaultRawType) {
  std::string mapFile = writeJmap();
  auto backend = boost::make_shared<TestableDummyBackend>(mapFile);
  backend->open();

  // A non-default raw type is obtained through the templated factory, like the default through a deduction-less call.
  auto raw = backend->getRawAccessor<int16_t>("MODULE", "SIXTEEN_BIT");

  BOOST_TEST(raw.getNumberOfElements() == 4U);

  raw[0] = int16_t(0x1234);
  raw[1] = int16_t(0x5678);
  raw[2] = int16_t(-1);
  raw[3] = int16_t(-32768);

  BOOST_TEST(raw[0].get() == int16_t(0x1234));
  BOOST_TEST(raw[1].get() == int16_t(0x5678));
  BOOST_TEST(raw[2].get() == int16_t(-1));
  BOOST_TEST(raw[3].get() == int16_t(-32768));

  // Value semantics with the narrower raw type.
  raw[0] += int16_t(1);
  BOOST_TEST(raw[0].get() == int16_t(0x1235));

  backend->close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// A deduction-less declaration uses the deduction guide and so resolves to 32-bit elements, like getRawAccessor.
BOOST_AUTO_TEST_CASE(testRawAccessorDeductionGuideUsesDefaultRawType) {
  std::string mapFile = writeJmap();
  auto backend = boost::make_shared<TestableDummyBackend>(mapFile);
  backend->open();

  // No explicit template argument: the deduction guide fixes RawType to int32_t. This only compiles if the guide
  // resolves to a 32-bit type, because the 4-byte register would fail the element-width assert otherwise.
  DummyRegisterRawAccessor deduced(backend, "MODULE", "MISALIGNED");
  static_assert(std::is_same_v<decltype(deduced), DummyRegisterRawAccessor<int32_t>>);

  deduced[0] = 0x0badf00d;
  BOOST_TEST(int(deduced[0]) == 0x0badf00d);

  backend->close();
  std::remove(mapFile.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
