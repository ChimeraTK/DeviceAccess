// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include "BackendFactory.h"
#include "Device.h"
#include "ProcessManagement.h"
#include "SharedDummyBackend.h"
#include "sharedDummyHelpers.h"
#include "Utilities.h"

#include <sys/wait.h>

#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/test/unit_test.hpp>

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace ChimeraTK;

// The chunk size is part of the public interface of DummyBackendBase, so the tests can build addresses relative to it
// without knowing its concrete value.
static constexpr uint64_t CHUNK = DummyBackendBase::CHUNK_SIZE;

/// Write a minimal jmap file to a temporary file and return its path. The map file defines a single register of the
/// given element count (and hence size) at the given bar and address. An explicit file name may be given to overwrite
/// an existing map file in place (e.g. to increase the bar sizes under the same name).
static std::string writeJmap(uint64_t bar, uint64_t address, uint64_t numberOfElements, std::string fileName = {}) {
  if(fileName.empty()) {
    fileName = "chunkedSharedDummyBackend_" + std::to_string(getpid()) + ".jmap";
  }
  std::ofstream file(fileName);
  file << R"({
  "mapFormatVersion": "1.0",
  "metadata": {},
  "interruptHandler": {},
  "addressSpace": {
    "MODULE": {
      "bytesPerElement": 4,
      "numberOfElements": )"
       << numberOfElements << R"(,
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

/// Full name of the shared-memory object holding the chunk (bar, chunkIndex) of the given instance.
static std::string chunkSegmentName(const std::string& baseName, uint64_t bar, uint64_t chunkIndex) {
  // Mirrors SharedDummyBackend's internal naming: <base>_BAR_<bar>_CHUNK_<index>.
  return baseName + "_BAR_" + std::to_string(bar) + "_CHUNK_" + std::to_string(chunkIndex);
}

/// Number of currently existing chunk segments of the given instance (any bar, any index).
static size_t countChunkSegments(const std::string& baseName) {
  size_t count = 0;
  const std::string prefix = baseName + "_BAR_";
  for(boost::filesystem::directory_iterator it("/dev/shm"), end; it != end; ++it) {
    const std::string fileName = it->path().filename().string();
    if(fileName.compare(0, prefix.size(), prefix) == 0) {
      ++count;
    }
  }
  return count;
}

/// A unique instanceIdHash so different test cases do not share a segment even if they use the same map file name.
static size_t uniqueInstanceHash(const std::string& tag) {
  return Utilities::shmDummyInstanceIdHash(tag + "_" + std::to_string(getpid()), {});
}

namespace {

  using namespace boost::unit_test_framework;

  static TestLocker testLocker("shareddummyTest.dmap");

  struct TestFixture {
    bool testRegisterNotInCatalogue(const std::string& registerPath) {
      // Also get the backend to test the catalogue
      auto backendInstance = BackendFactory::getInstance().createBackend("SHDMEMDEV");

      auto catalogue = backendInstance->getRegisterCatalogue();

      // the register is not in the iterable catalogue (might be a hidden register though)
      bool found = false;
      for(auto& info : catalogue) {
        if(info.getRegisterName() == registerPath) {
          found = true;
          break;
        }
      }

      return !found;
    }
  };

  BOOST_AUTO_TEST_SUITE(SharedDummyBackendTestSuite)

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testOpenClose) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
    dev.open();
    BOOST_CHECK(dev.isOpened());
    // you must always be able to call open  and close again
    dev.open();
    BOOST_CHECK(dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testReadWrite) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());

    // Write/read some values to/from the shared memory
    ChimeraTK::OneDRegisterAccessor<int> processVars11 = dev.getOneDRegisterAccessor<int>("FEATURE1/AREA");
    int n = 0;
    std::generate(processVars11.begin(), processVars11.end(), [n]() mutable { return n++; });
    processVars11.write();
    processVars11.read();

    ChimeraTK::OneDRegisterAccessor<int> processVars23 = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA3");
    n = 0;
    std::generate(processVars23.begin(), processVars23.end(), [n]() mutable { return n++; });
    processVars23.write();
    processVars23.read();

    // Write to memory and check values mirrored by another process
    ChimeraTK::OneDRegisterAccessor<int> processVarsWrite21 = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    n = 0;
    std::generate(processVarsWrite21.begin(), processVarsWrite21.end(), [n]() mutable { return n++; });
    processVarsWrite21.write();

    // start second accessing application
    BOOST_CHECK(!std::system("./testSharedDummyBackendExt "
                             "--run_test=SharedDummyBackendTestSuite/testReadWrite"));

    // Check if values have been written back by the other application
    ChimeraTK::OneDRegisterAccessor<int> processVarsRead = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
    processVarsRead.read();

    BOOST_CHECK(static_cast<std::vector<int>>(processVarsWrite21) == static_cast<std::vector<int>>(processVarsRead));
    dev.close();
  }

  /********************************************************************************************************************/

  BOOST_FIXTURE_TEST_CASE(testWriteToReadOnly, TestFixture) {
    setDMapFilePath("shareddummyTest.dmap");
    Device dev;
    dev.open("SHDMEMDEV");

    ScalarRegisterAccessor<int> roRegisterOne{dev.getScalarRegisterAccessor<int>("WORD_READ_ONLY_1")};
    ScalarRegisterAccessor<int> roRegisterTwo_dw{
        dev.getScalarRegisterAccessor<int>("WORD_READ_ONLY_2.DUMMY_WRITEABLE")};

    BOOST_CHECK(roRegisterOne.isReadOnly());
    BOOST_CHECK(!roRegisterOne.isWriteable());
    BOOST_CHECK(!roRegisterTwo_dw.isReadOnly());
    BOOST_CHECK(roRegisterTwo_dw.isWriteable());

    BOOST_CHECK(testRegisterNotInCatalogue("WORD_READ_ONLY_2.DUMMY_WRITEABLE"));

    BOOST_CHECK_THROW(roRegisterOne.write(), ChimeraTK::logic_error);

    roRegisterOne = 0;
    roRegisterTwo_dw = 25;
    roRegisterTwo_dw.write();

    // Start second accessing application
    // This is complementary and has a writeable accessor to WORD_READ_ONLY_1
    // to which it mirrors the value of the second register
    BOOST_CHECK(!std::system("./testSharedDummyBackendExt "
                             "--run_test=SharedDummyBackendTestSuite/testWriteToReadOnly"));

    roRegisterOne.read();
    BOOST_CHECK_EQUAL(roRegisterTwo_dw, roRegisterOne);

    dev.close();
  }

  /********************************************************************************************************************/

  BOOST_FIXTURE_TEST_CASE(testCreateBackend, TestFixture) {
    setDMapFilePath("shareddummyTest.dmap");
    auto backendInst1 = BackendFactory::getInstance().createBackend("SHDMEMDEV");
    auto backendInst2 = BackendFactory::getInstance().createBackend("SHDMEMDEV");
    auto backendInst3 = BackendFactory::getInstance().createBackend("SHDMEMDEV2");

    BOOST_CHECK(backendInst1.get() == backendInst2.get());
    BOOST_CHECK(backendInst3.get() != backendInst2.get());
  }

  /********************************************************************************************************************/

  // A map file declaring an unreasonably huge address space (1000 TB) opens and works while only the first and the last
  // few bytes are touched. Reserving the full address space up front would exhaust any shared memory, so succeeding
  // proves the storage is allocated lazily.
  BOOST_AUTO_TEST_CASE(testHugeAddressSpaceMaterialisesOnlyTouchedChunks) {
    const uint64_t numberOfElements = 1000ULL * 1000 * 1000 * 1000 * 1000 / 4; // 1000 TB at 4 bytes per element
    std::string mapFile = writeJmap(0, 0, numberOfElements);

    const size_t instanceHash = uniqueInstanceHash("huge");
    const std::string baseName = SharedDummyBackend::getSharedMemoryName(instanceHash, mapFile, getUserName());

    {
      SharedDummyBackend backend(instanceHash, mapFile);
      backend.open();

      // Only the home segment exists before any access.
      BOOST_TEST(countChunkSegments(baseName) == 0U);

      const uint64_t bar = 0;
      const uint64_t firstAddress = 0;
      const uint64_t lastAddress = numberOfElements * 4 - 4;

      int32_t first = 0x11223344;
      backend.write(bar, firstAddress, &first, sizeof(int32_t));

      int32_t last = 0x55667788;
      backend.write(bar, lastAddress, &last, sizeof(int32_t));

      // The first and the last bytes read back correctly.
      int32_t readBack = 0;
      backend.read(bar, firstAddress, &readBack, sizeof(int32_t));
      BOOST_TEST(readBack == 0x11223344);
      backend.read(bar, lastAddress, &readBack, sizeof(int32_t));
      BOOST_TEST(readBack == 0x55667788);

      // The two writes materialised exactly two chunks, far apart in the address space.
      BOOST_TEST(countChunkSegments(baseName) == 2U);
      BOOST_TEST(shm_exists(chunkSegmentName(baseName, bar, 0)));
      BOOST_TEST(shm_exists(chunkSegmentName(baseName, bar, lastAddress / CHUNK)));

      // After the last process leaves, no chunk segment is left behind.
      backend.close();
    }

    BOOST_TEST(countChunkSegments(baseName) == 0U);
    BOOST_TEST(!shm_exists(baseName));

    std::remove(mapFile.c_str());
  }

  /********************************************************************************************************************/

  // The chunk chain is walked correctly for many chunks in several bars, and after the last process leaves (also after
  // a killed process is detected) no chunk segment of the instance is left in shared memory: the last process leaving
  // removes every segment, so reopening cannot share materialised chunks either.
  BOOST_AUTO_TEST_CASE(testChainCleanupRemovesAllChunks) {
    std::string mapFile = writeJmap(0, 0, 4);
    const size_t instanceHash = uniqueInstanceHash("cleanup");
    const std::string baseName = SharedDummyBackend::getSharedMemoryName(instanceHash, mapFile, getUserName());

    {
      SharedDummyBackend backend(instanceHash, mapFile);
      backend.open();

      int32_t word = 0x5a5a5a5a;
      // Materialise 5 chunks across 3 different bars, interleaved so the chain is not sorted by bar or address.
      // Typed arguments avoid the ambiguity between the uint8_t/uint32_t and uint64_t/uint64_t overloads.
      const uint64_t bar0 = 0;
      const uint64_t bar1 = 1;
      const uint64_t bar2 = 2;
      const uint64_t addr0 = 0;
      backend.write(bar0, addr0, &word, sizeof(int32_t));
      backend.write(bar1, 2 * CHUNK, &word, sizeof(int32_t));
      backend.write(bar0, 4 * CHUNK, &word, sizeof(int32_t));
      backend.write(bar2, CHUNK, &word, sizeof(int32_t));
      backend.write(bar0, 7 * CHUNK, &word, sizeof(int32_t));

      BOOST_TEST(countChunkSegments(baseName) == 5U);

      backend.close();
    }

    BOOST_TEST(countChunkSegments(baseName) == 0U);
    BOOST_TEST(!shm_exists(baseName));

    std::remove(mapFile.c_str());
  }

  /********************************************************************************************************************/

  // A write to a chunk far beyond the map-defined range is cleaned up again when the last process leaves: every chunk
  // segment of the instance is removed, so no leftover segment remains. Sharing between instances is not exercised
  // here, because the last process leaving removes the segments (reopening cannot see them); the forked
  // testJoinSegmentWithIncreasedBarSizes covers multi-process sharing.
  BOOST_AUTO_TEST_CASE(testCleanupRemovesChunkBeyondMapRange) {
    std::string mapFile = writeJmap(0, 0, 4);
    const size_t instanceHash = uniqueInstanceHash("shared");
    const std::string baseName = SharedDummyBackend::getSharedMemoryName(instanceHash, mapFile, getUserName());

    {
      SharedDummyBackend backend(instanceHash, mapFile);
      backend.open();
      int32_t word = 0x0badf00d;
      const uint64_t bar = 0;
      backend.write(bar, 3 * CHUNK + 16, &word, sizeof(int32_t));
      backend.close();
    }

    // The last process leaving removes every chunk segment.
    BOOST_TEST(countChunkSegments(baseName) == 0U);

    std::remove(mapFile.c_str());
  }

  /********************************************************************************************************************/

  // Any address is valid: writing far beyond the map-defined size materialises chunks on demand and reads them back.
  BOOST_AUTO_TEST_CASE(testWriteBeyondMapDefinedSize) {
    std::string mapFile = writeJmap(0, 0, 4);
    const size_t instanceHash = uniqueInstanceHash("beyond");
    const std::string baseName = SharedDummyBackend::getSharedMemoryName(instanceHash, mapFile, getUserName());

    {
      SharedDummyBackend backend(instanceHash, mapFile);
      backend.open();

      const uint64_t bar = 5;
      const uint64_t farAddress = 1234567 * CHUNK + 8;

      int32_t word = -1;
      backend.read(bar, farAddress, &word, sizeof(int32_t));
      BOOST_TEST(word == 0);

      word = 0x0badf00d;
      backend.write(bar, farAddress, &word, sizeof(int32_t));

      word = 0;
      backend.read(bar, farAddress, &word, sizeof(int32_t));
      BOOST_TEST(word == 0x0badf00d);

      BOOST_TEST(countChunkSegments(baseName) == 1U);

      backend.close();
    }

    BOOST_TEST(countChunkSegments(baseName) == 0U);

    std::remove(mapFile.c_str());
  }

  /********************************************************************************************************************/

  // A process joining the same segment with a map file of the same name but increased bar sizes works without crash or
  // confusing errors, and reads the data written by the first process back. The segment name depends only on the map
  // file name, not on its content, so both processes derive the same name.
  BOOST_AUTO_TEST_CASE(testJoinSegmentWithIncreasedBarSizes) {
    std::string mapFile = writeJmap(0, 0, 4);
    const size_t instanceHash = uniqueInstanceHash("increased");
    const std::string baseName = SharedDummyBackend::getSharedMemoryName(instanceHash, mapFile, getUserName());

    const uint64_t bar = 0;
    const uint64_t address = 4;
    const uint64_t highAddress = size_t{1023} * 4;

    {
      SharedDummyBackend backend(instanceHash, mapFile);
      backend.open();

      // The first process writes a value at an address which is valid in both map versions.
      int32_t word = 0x0badf00d;
      backend.write(bar, address, &word, sizeof(int32_t));

      pid_t child = fork();
      BOOST_REQUIRE(child >= 0);

      if(child == 0) {
        // Child: rewrite the map file with increased bar sizes under the same name, then join the segment.
        int exitCode = 0;
        try {
          writeJmap(0, 0, 1024, mapFile);

          SharedDummyBackend childBackend(instanceHash, mapFile);
          childBackend.open();

          // The value written by the parent is visible.
          int32_t readBack = 0;
          childBackend.read(bar, address, &readBack, sizeof(int32_t));
          if(readBack != 0x0badf00d) {
            exitCode = 1;
          }

          // A write through the increased-size map works as well.
          int32_t other = 0x12345678;
          childBackend.write(bar, highAddress, &other, sizeof(int32_t));
        }
        catch(const std::exception&) {
          exitCode = 2;
        }

        // _exit to avoid running the destructors of the inherited parent objects.
        std::_Exit(exitCode);
      }

      int status = 0;
      BOOST_REQUIRE(waitpid(child, &status, 0) == child);
      BOOST_TEST(WIFEXITED(status));
      BOOST_TEST(WEXITSTATUS(status) == 0);

      // The parent still sees its own value and the child's write to the increased range.
      int32_t readBack = 0;
      backend.read(bar, address, &readBack, sizeof(int32_t));
      BOOST_TEST(readBack == 0x0badf00d);

      int32_t other = 0;
      backend.read(bar, highAddress, &other, sizeof(int32_t));
      BOOST_TEST(other == 0x12345678);

      backend.close();
    }

    // After the last process leaves, the child's stale pid is detected and every segment of the instance is removed.
    BOOST_TEST(countChunkSegments(baseName) == 0U);
    BOOST_TEST(!shm_exists(baseName));

    std::remove(mapFile.c_str());
  }

  /********************************************************************************************************************/

  // A process killed between writing a chain link and creating its segment leaves a dangling link. Cleanup must skip
  // such a name instead of mapping it with open_only, which would throw a boost interprocess_exception and reach the
  // std::terminate() handler. This reproduces the dangling link by materialising two chained chunks and then removing
  // the second segment behind the backend's back; close() must tolerate the resulting dangling successor link, remove
  // the still-existing first chunk and leave nothing behind.
  BOOST_AUTO_TEST_CASE(testDanglingChainLinkIsToleratedOnCleanup) {
    std::string mapFile = writeJmap(0, 0, 4);
    const size_t instanceHash = uniqueInstanceHash("dangling");
    const std::string baseName = SharedDummyBackend::getSharedMemoryName(instanceHash, mapFile, getUserName());

    {
      SharedDummyBackend backend(instanceHash, mapFile);
      backend.open();

      // Materialise two chained chunks: bar 0/chunk 0 (head) and bar 0/chunk 1 (tail, the head's successor).
      int32_t word = 0x0badf00d;
      const uint64_t bar = 0;
      backend.write(bar, 0, &word, sizeof(int32_t));
      backend.write(bar, CHUNK, &word, sizeof(int32_t));
      BOOST_TEST(countChunkSegments(baseName) == 2U);

      // Remove the successor segment behind the backend's back: the head's link now names a non-existent segment,
      // exactly as a process killed between writing the link and creating the segment would leave it.
      const std::string successorName = chunkSegmentName(baseName, bar, 1);
      BOOST_REQUIRE(shm_exists(successorName));
      BOOST_TEST(boost::interprocess::shared_memory_object::remove(successorName.c_str()));
      BOOST_TEST(countChunkSegments(baseName) == 1U);

      // close() walks the chain from the head; the dangling link must end the walk silently.
      backend.close();
    }

    // The still-existing chunk was removed, the dangling name never existed, and nothing is left behind.
    BOOST_TEST(countChunkSegments(baseName) == 0U);
    BOOST_TEST(!shm_exists(baseName));

    std::remove(mapFile.c_str());
  }

} // anonymous namespace

/**********************************************************************************************************************/
BOOST_AUTO_TEST_SUITE_END()
