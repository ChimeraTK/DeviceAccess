// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMathPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "LogicalNameMappingBackend.h"

using namespace ChimeraTK;

#define CHECK_TIMEOUT(execPreCheck, condition, maxMilliseconds)                                                        \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    execPreCheck while(!(condition)) {                                                                                 \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) {                                                                                            \
        std::cout << "failed condition: " << #condition << std::endl;                                                  \
        break;                                                                                                         \
      }                                                                                                                \
      usleep(100000);                                                                                                  \
      execPreCheck                                                                                                     \
    }                                                                                                                  \
  }

BOOST_AUTO_TEST_SUITE(LMapMathPluginTestSuite)

/********************************************************************************************************************/

struct DummyForCleanupCheck : public LogicalNameMappingBackend {
  using LogicalNameMappingBackend::LogicalNameMappingBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string>) {
    return boost::make_shared<DummyForCleanupCheck>("mathPluginWithPushPars.xlmap");
  }
  ~DummyForCleanupCheck() override {
    std::cout << "~DummyForCleanupCheck()" << std::endl;
    cleanupCalled = true;
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "DummyForCleanupCheck", &DummyForCleanupCheck::createInstance, {"map"});
    }
  };
  static std::atomic_bool cleanupCalled;
};
std::atomic_bool DummyForCleanupCheck::cleanupCalled;
static DummyForCleanupCheck::BackendRegisterer gDFCCRegisterer;

BOOST_AUTO_TEST_CASE(testPushPars) {
  setDMapFilePath("mathPluginWithPushPars-testCleanup.dmap");
  {
    ChimeraTK::Device targetDevice;
    targetDevice.open("HOLD");

    auto accTarget = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/WORD_G");
    auto pollPar = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/POLLPAR");
    pollPar = 1;
    pollPar.write();

    ChimeraTK::Device device;
    device.open("EOD");
    device.activateAsyncRead();
    auto scalarPar = device.getScalarRegisterAccessor<uint32_t>("DET/EXPOSURE");

    scalarPar = 2;
    scalarPar.write();

    // check that even if main value (x in formula) is not written, writing parameter should give proper result
    int accMathWrite0 = 0; // initial value for "x"

    CHECK_TIMEOUT(accTarget.readLatest();
                  , int(accTarget) == 100 * (int)pollPar + 10 * (int)scalarPar + accMathWrite0, 5000);

    // write to main value and check result
    auto accMathWrite = device.getScalarRegisterAccessor<double>("DET/GAIN");
    accMathWrite = 3;
    accMathWrite.write();
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)scalarPar + (int)accMathWrite);

    // write to push-parameter and check result
    // note, it's a new feature that result is completely written when write() returns.
    scalarPar = 4;
    scalarPar.write();
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)scalarPar + (int)accMathWrite);
  }
  // regression test for bug https://redmine.msktools.desy.de/issues/11506
  // (math plugin + push-parameter + shm has resource cleanup problem)
  BOOST_CHECK(DummyForCleanupCheck::cleanupCalled);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
