// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include "Device.h"
#include "ProcessManagement.h"
#include "sharedDummyHelpers.h"
#include "Utilities.h"

#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

  using namespace ChimeraTK;
  using namespace boost::unit_test_framework;

  // Static function prototypes
  static void interrupt_handler(int);

  // Static variables
  //  Use hardcoded information from the dmap-file to
  //  only use public interface here
  static std::string instanceId{"1"};
  static std::string mapFileName{"shareddummy.map"};

  static bool terminationCaught = false;

  BOOST_AUTO_TEST_SUITE(SharedDummyBackendTestSuite)

  struct TestFixture {
    TestFixture() : argc(framework::master_test_suite().argc), argv(framework::master_test_suite().argv) {}

    int argc;
    char** argv;
  };

  /********************************************************************************************************************/

  BOOST_FIXTURE_TEST_CASE(testRobustnessMain, TestFixture) {
    unsigned nIterations = 0;
    if(argc != 2) {
      std::cout << "Illegal number of arguments. Test case must be called with "
                   "the number of read/write cycles!"
                << std::endl;
    }
    else {
      nIterations = atoi(argv[1]);
    }

    setDMapFilePath("shareddummyTest.dmap");

    bool readbackCorrect = false;
    bool waitingForResponse = true;
    const unsigned maxIncorrectIterations = 20; /* Timeout while waiting for 2nd application */
    unsigned iterations = 0;
    unsigned incorrectIterations = 0;

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());

    ChimeraTK::OneDRegisterAccessor<int> processVarsWrite = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    std::vector<int> processVarsOld(processVarsWrite.getNElements(), 0);

    do {
      // Write some values to the shared memory
      for(size_t i = 0; i < processVarsWrite.getNElements(); ++i) {
        processVarsWrite[i] = i + iterations;
      }
      processVarsWrite.write();

      // Check if values have been written back by the other application
      ChimeraTK::OneDRegisterAccessor<int> processVarsRead = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
      // Read as long as the readback from last iteration has been overwritten
      do {
        processVarsRead.read();
      } while((std::vector<int>)processVarsRead == (std::vector<int>)processVarsOld && !waitingForResponse);

      if((std::vector<int>)processVarsWrite == (std::vector<int>)processVarsRead) {
        if(waitingForResponse) {
          waitingForResponse = false;
        }
        readbackCorrect = true;
      }
      else {
        readbackCorrect = false;
        if(!waitingForResponse) {
          std::cout << "Corrupted data detected:" << std::endl;
          for(const auto pvr : processVarsRead) {
            std::cout << "    " << pvr << std::endl;
          }
        }
      }

      if(waitingForResponse) {
        incorrectIterations++;
      }
      else {
        iterations++;
      }
      processVarsOld = processVarsWrite;
    } while((readbackCorrect || waitingForResponse) && incorrectIterations != maxIncorrectIterations &&
        iterations != nIterations);

    BOOST_CHECK(readbackCorrect);
    std::cout << "Finished test after " << iterations << " of " << nIterations << " Iterations." << std::endl;
    if(incorrectIterations == maxIncorrectIterations) {
      std::cout << "Test cancelled because echoing process did not respond!" << std::endl;
    }

    dev.close();
  }

  /**
   * This test case implements a second application accessing the shared memory
   * which mirrors the values to another register bar.
   * For a robustness test, it can be called with the argument "KEEP_RUNNING", so
   * that it constantly operates on the shared memory. In this case, it can be
   * terminated gracefully by sending SIGINT.
   */
  BOOST_FIXTURE_TEST_CASE(testReadWrite, TestFixture) {
    signal(SIGINT, interrupt_handler);

    bool keepRunning = false;

    if(argc >= 2 && !strcmp(argv[1], "KEEP_RUNNING")) {
      keepRunning = true;
    }

    setDMapFilePath("shareddummyTest.dmap");

    boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);

    std::string shmName{createExpectedShmName(instanceId, absPathToMapFile.string(), getUserName())};

    {
      Device dev;
      BOOST_CHECK(!dev.isOpened());
      dev.open("SHDMEMDEV");
      BOOST_CHECK(dev.isOpened());

      BOOST_CHECK(shm_exists(shmName));

      do {
        ChimeraTK::OneDRegisterAccessor<int> processVarsRead = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
        processVarsRead.read();

        ChimeraTK::OneDRegisterAccessor<int> processVarsWrite = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");

        for(size_t i = 0; i < processVarsRead.getNElements(); ++i) {
          processVarsWrite[i] = processVarsRead[i];
        }
        processVarsWrite.write();
      } while(keepRunning && !terminationCaught);
      dev.close();
    }
  }

  /**
   * This test case implements a second application accessing the shared memory
   * for testing the ".DUMMY_WRITEABLE" feature.
   *
   * This is called from the complementary automatic test case.
   */
  BOOST_AUTO_TEST_CASE(testWriteToReadOnly) {
    setDMapFilePath("shareddummyTest.dmap");
    Device dev;
    dev.open("SHDMEMDEV");

    ScalarRegisterAccessor<int> roRegisterOne_dw{
        dev.getScalarRegisterAccessor<int>("WORD_READ_ONLY_1.DUMMY_WRITEABLE")};
    ScalarRegisterAccessor<int> roRegisterTwo{dev.getScalarRegisterAccessor<int>("WORD_READ_ONLY_2")};

    BOOST_CHECK(!roRegisterOne_dw.isReadOnly());
    BOOST_CHECK(roRegisterOne_dw.isWriteable());
    BOOST_CHECK(roRegisterTwo.isReadOnly());
    BOOST_CHECK(!roRegisterTwo.isWriteable());

    BOOST_CHECK_THROW(roRegisterTwo.write(), ChimeraTK::logic_error);

    // Mirror to the other register
    roRegisterTwo.read();
    const int roRegisterTwoValue = roRegisterTwo;
    roRegisterOne_dw = roRegisterTwoValue;
    roRegisterOne_dw.write();

    // Check happens in the automativ test case.

    dev.close();
  }

  /**
   * This test is called from the script testing the process ID
   * management. It writes some values to the shared memory and
   * is to be killed from the script so that the shared memory does
   * not get removed properly. This should then be achieved by
   * "testVerifyCleanup".
   */
  BOOST_AUTO_TEST_CASE(testMakeMess) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    dev.open("SHDMEMDEV");

    ChimeraTK::OneDRegisterAccessor<int> processVars = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");

    int n = 0;
    std::generate(processVars.begin(), processVars.end(), [n]() mutable { return n++; });
    processVars.write();

    // Wait to be killed or timeout...
    std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(20));

    dev.close();
  }

  /**
   * This test is called from the script testing the process ID
   * management. It verifies that the registers do not contain
   * any nonzero content that was written by another process.
   */
  BOOST_AUTO_TEST_CASE(testVerifyCleanup) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    dev.open("SHDMEMDEV");

    ChimeraTK::OneDRegisterAccessor<int> processVars = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    processVars.read();

    // Check if content is all zero
    BOOST_CHECK(std::all_of(processVars.begin(), processVars.end(), [](int i) { return i == 0; }));

    dev.close();
  }

  /**
   * Just checks if the shared memory has been removed.
   */
  BOOST_AUTO_TEST_CASE(testVerifyMemoryDeleted) {
    setDMapFilePath("shareddummyTest.dmap");

    boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);

    std::string shmName{createExpectedShmName(instanceId, absPathToMapFile.string(), getUserName())};

    // Test if memory is removed
    BOOST_CHECK(!shm_exists(shmName));
  }

  /********************************************************************************************************************/
  BOOST_AUTO_TEST_SUITE_END()

  /**
   * Catch interrupt signal, so we can terminate the test and still
   * clean up the shared memory.
   */
  static void interrupt_handler(int signal) {
    std::cout << "Caught interrupt signal (" << signal << "). Terminating..." << std::endl;
    terminationCaught = true;
  }

} // anonymous namespace
