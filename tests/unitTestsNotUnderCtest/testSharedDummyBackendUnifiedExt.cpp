// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendUnifiedTest
#include "Device.h"
#include "ProcessManagement.h"
#include "sharedDummyHelpers.h"
#include "Utilities.h"

#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <string>
#include <thread>
#include <unistd.h>

namespace {

  using namespace ChimeraTK;
  using namespace boost::unit_test_framework;

  // Static variables
  //  Use hardcoded information from the dmap-file to
  //  only use public interface here
  static std::string instanceId{"1"};
  static std::string mapFileName{"sharedDummyUnified.map"};

  BOOST_AUTO_TEST_SUITE(SharedDummyBackendUnifiedTestSuite)

  struct TestFixture {
    TestFixture() : argc(framework::master_test_suite().argc), argv(framework::master_test_suite().argv) {}

    int argc;
    char** argv;
  };

  static void mirrorArea(
      ChimeraTK::OneDRegisterAccessor<int>& processVarsTo, ChimeraTK::OneDRegisterAccessor<int>& processVarsFrom) {
    processVarsFrom.read();
    for(size_t i = 0; i < processVarsFrom.getNElements(); ++i) {
      processVarsTo[i] = processVarsFrom[i];
    }
    processVarsTo.write();
  }

  /********************************************************************************************************************/

  /**
   * This test case implements a second application accessing the shared memory
   * which mirrors the values to another region.
   * Mirroring happens on request and the direction needs to be specified.
   * The test runs until either stopped by a request via shm or interrupted by a signal.
   */
  BOOST_FIXTURE_TEST_CASE(testRegisterAccessor, TestFixture) {
    bool keepRunning = true;

    setDMapFilePath("sharedDummyUnified.dmap");

    boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);
    std::string shmName{createExpectedShmName(instanceId, absPathToMapFile.string(), getUserName())};

    {
      Device dev;
      BOOST_CHECK(!dev.isOpened());
      dev.open("SHDMEMDEV");
      BOOST_CHECK(dev.isOpened());

      BOOST_CHECK(shm_exists(shmName));

      ChimeraTK::OneDRegisterAccessor<int> processVarsFeature = dev.getOneDRegisterAccessor<int>("FEATURE/AREA1");

      ChimeraTK::OneDRegisterAccessor<int> processVarsMirror = dev.getOneDRegisterAccessor<int>("MIRRORED/AREA1");

      BOOST_CHECK_EQUAL(processVarsFeature.getNElements(), processVarsMirror.getNElements());

      ScalarRegisterAccessor<int> mirrorRequest_Type = dev.getScalarRegisterAccessor<int>("MIRRORREQUEST/TYPE");
      ScalarRegisterAccessor<int> mirrorRequest_Busy = dev.getScalarRegisterAccessor<int>("MIRRORREQUEST/BUSY");
      ScalarRegisterAccessor<int> mirrorRequest_Updated =
          dev.getScalarRegisterAccessor<int>("MIRRORREQUEST/UPDATED/DUMMY_WRITEABLE");
      ScalarRegisterAccessor<int> mirrorRequest_DataInterrupt =
          dev.getScalarRegisterAccessor<int>("MIRRORREQUEST/DATA_INTERRUPT");
      ScalarRegisterAccessor<int> mirrorRequestUpdated_Interrupt{
          dev.getScalarRegisterAccessor<int>("DUMMY_INTERRUPT_0")};

      ScalarRegisterAccessor<int> dataInterrupt{dev.getScalarRegisterAccessor<int>("DUMMY_INTERRUPT_1")};

      do {
        // poll Busy until it is set to true, indicating a new request
        do {
          mirrorRequest_Busy.readLatest();
          // we use boost::sleep because it defines an interruption point for signals
          boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
        } while(mirrorRequest_Busy == 0);

        mirrorRequest_Type.readLatest();
        switch(static_cast<MirrorRequestType>((int)mirrorRequest_Type)) {
          case MirrorRequestType::from:
            mirrorArea(processVarsMirror, processVarsFeature);
            break;
          case MirrorRequestType::to:
            mirrorArea(processVarsFeature, processVarsMirror);
            break;
          case MirrorRequestType::stop:
            keepRunning = false;
        }
        mirrorRequest_Updated.readLatest();
        ++mirrorRequest_Updated;
        mirrorRequest_Updated.write();
        // also trigger interrupt for this variable
        mirrorRequestUpdated_Interrupt = 1;
        mirrorRequestUpdated_Interrupt.write();

        mirrorRequest_Busy = 0;
        mirrorRequest_Busy.write();

        mirrorRequest_DataInterrupt.readLatest();
        if(mirrorRequest_DataInterrupt == 1) {
          dataInterrupt.write();
          mirrorRequest_DataInterrupt = 0;
          mirrorRequest_DataInterrupt.write();
        }
      } while(keepRunning);
      dev.close();
    }
  }

  /********************************************************************************************************************/
  BOOST_AUTO_TEST_SUITE_END()

} // anonymous namespace
