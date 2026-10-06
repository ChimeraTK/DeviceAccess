// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ChunkedDummyBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyBackend.h"
#include "Exception.h"

#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace ChimeraTK;

// The chunk size is part of the public interface of DummyBackendBase, so the tests can build addresses relative to it
// without knowing its concrete value.
static constexpr uint64_t CHUNK = DummyBackendBase::CHUNK_SIZE;

/// Write a minimal jmap file to a temporary file and return its path. The map file defines a single 4-byte register
/// at the given bar and address.
static std::string writeSingleRegisterJmap(uint64_t bar, uint64_t address) {
  std::string fileName = "chunkedDummyBackend_" + std::to_string(getpid()) + ".jmap";
  std::ofstream file(fileName);
  file << R"({
  "mapFormatVersion": "1.0",
  "metadata": {},
  "interruptHandler": {},
  "addressSpace": {
    "MODULE": {
      "bytesPerElement": 4,
      "numberOfElements": 1,
      "address": {
        "type": "IO",
        "channel": )"
       << bar << R"(,
        "offset": )"
       << address << R"(
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

  /// Test wrapper giving access to the internal chunk map so a test can check that only the touched chunks were
  /// materialised.
  class TestableDummyBackend : public DummyBackend {
   public:
    explicit TestableDummyBackend(const std::string& mapFileName) : DummyBackend(mapFileName) {}

    using DummyBackend::_barContents;

    /// Number of materialised chunks for the given bar.
    size_t chunkCount(uint64_t bar) {
      size_t count = 0;
      for(const auto& entry : _barContents) {
        if(entry.first.first == bar) {
          ++count;
        }
      }
      return count;
    }
  };

} // namespace

BOOST_AUTO_TEST_SUITE(ChunkedDummyBackendTestSuite)

/**********************************************************************************************************************/

// A transfer that crosses a chunk boundary is stitched together correctly, and the untouched neighbouring chunks stay
// zero.
BOOST_AUTO_TEST_CASE(testWriteReadAcrossChunkBoundary) {
  std::string mapFile = writeSingleRegisterJmap(0, 0);
  TestableDummyBackend backend(mapFile);
  backend.open();

  // Write a word that straddles the boundary between chunk 0 and chunk 1.
  const uint64_t address = CHUNK - 2 * sizeof(int32_t);
  std::vector<int32_t> data{0x11112222, 0x33334444, 0x55556666, 0x77778888};
  const uint64_t bar = 0;
  backend.write(bar, address, data.data(), data.size() * sizeof(int32_t));

  std::vector<int32_t> readBack(data.size(), 0);
  backend.read(bar, address, readBack.data(), readBack.size() * sizeof(int32_t));
  BOOST_TEST(readBack == data, boost::test_tools::per_element());

  // The two touched chunks exist, nothing else.
  BOOST_TEST(backend.chunkCount(0) == 2U);

  // Bytes before the written range within chunk 0 are still zero.
  int32_t before = -1;
  backend.read(bar, address - sizeof(int32_t), &before, sizeof(int32_t));
  BOOST_TEST(before == 0);

  // Bytes after the written range within chunk 1 are still zero.
  int32_t after = -1;
  backend.read(bar, address + data.size() * sizeof(int32_t), &after, sizeof(int32_t));
  BOOST_TEST(after == 0);

  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// Reading a never-allocated chunk yields zeros and materialises exactly that one chunk.
BOOST_AUTO_TEST_CASE(testReadUntouchedChunkYieldsZeros) {
  std::string mapFile = writeSingleRegisterJmap(0, 0);
  TestableDummyBackend backend(mapFile);
  backend.open();

  // Read far beyond the map-defined range from a chunk that was never touched.
  int32_t word = -1;
  const uint64_t bar = 0;
  backend.read(bar, 10 * CHUNK, &word, sizeof(int32_t));
  BOOST_TEST(word == 0);

  // Reading materialises the chunk (a read materialises missing chunks too), but only that single chunk.
  BOOST_TEST(backend.chunkCount(0) == 1U);

  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// A write materialises only the touched chunk.
BOOST_AUTO_TEST_CASE(testWriteMaterialisesOnlyTouchedChunk) {
  std::string mapFile = writeSingleRegisterJmap(0, 0);
  TestableDummyBackend backend(mapFile);
  backend.open();

  int32_t word = 42;
  const uint64_t bar = 0;
  backend.write(bar, 3 * CHUNK + 8, &word, sizeof(int32_t));

  BOOST_TEST(backend.chunkCount(0) == 1U);

  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// Any address is valid: reads beyond the map-defined bar size yield zeros, writes materialise chunks on demand, and
// the same holds for an arbitrary bar.
BOOST_AUTO_TEST_CASE(testAccessBeyondMapDefinedRange) {
  std::string mapFile = writeSingleRegisterJmap(0, 0);
  TestableDummyBackend backend(mapFile);
  backend.open();

  const uint64_t farAddress = 1234567 * CHUNK + 4;

  int32_t word = -1;
  const uint64_t bar = 7;
  backend.read(bar, farAddress, &word, sizeof(int32_t));
  BOOST_TEST(word == 0);

  word = 0x0badf00d;
  backend.write(bar, farAddress, &word, sizeof(int32_t));

  word = 0;
  backend.read(bar, farAddress, &word, sizeof(int32_t));
  BOOST_TEST(word == 0x0badf00d);

  // Only bar 7 got a chunk, and only one.
  BOOST_TEST(backend.chunkCount(7) == 1U);
  BOOST_TEST(backend.chunkCount(0) == 0U);

  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// Chunks of different bars are independent, even at the same offset.
BOOST_AUTO_TEST_CASE(testBarsAreIndependent) {
  std::string mapFile = writeSingleRegisterJmap(0, 0);
  TestableDummyBackend backend(mapFile);
  backend.open();

  int32_t word = 0x12345678;
  const uint64_t bar = 0;
  const uint64_t address = 0;
  backend.write(bar, address, &word, sizeof(int32_t));

  int32_t other = -1;
  const uint64_t otherBar = 1;
  backend.read(otherBar, address, &other, sizeof(int32_t));
  BOOST_TEST(other == 0);

  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// The check that the transfer size is a multiple of the word size is unchanged.
BOOST_AUTO_TEST_CASE(testTransferSizeMustBeMultipleOfWordSize) {
  std::string mapFile = writeSingleRegisterJmap(0, 0);
  TestableDummyBackend backend(mapFile);
  backend.open();

  uint64_t bar = 0;
  uint64_t address = 0;
  int32_t word = 0;
  BOOST_CHECK_THROW(backend.read(bar, address, &word, 3), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(backend.write(bar, address, &word, 3), ChimeraTK::logic_error);

  std::remove(mapFile.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
