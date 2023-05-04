// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMathPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "LogicalNameMappingBackend.h"

using namespace ChimeraTK;

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
  setDMapFilePath("mathPluginWithPushPars.dmap");
  {
    // we enforce that initial values from variable definition in xlmap are never used in MathPugin
    // - therefore, in test here, we must issue write for all push-parameters
    // - then, version number check in MathPlugin will notice valid data has been provided after open
    // - for poll-type accessors, version number check also succeeds since they get recent version numbers anyway
    // Test needs to call activateAsyncRead but it must be irrelevant whether before or after writes

    ChimeraTK::Device targetDevice;
    targetDevice.open("HOLD");

    auto accTarget = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/WORD_G");
    auto pollPar = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/POLLPAR");
    pollPar = 1;
    pollPar.write();

    ChimeraTK::Device logicalDevice("EOD");
    logicalDevice.open();
    logicalDevice.activateAsyncRead();
    auto pushPar = logicalDevice.getScalarRegisterAccessor<uint32_t>("DET/EXPOSURE");

    pushPar = 2;
    pushPar.write();

    auto accMathWrite = logicalDevice.getScalarRegisterAccessor<double>("DET/GAIN");
    // we don't have main value (x in formula) yet, since it wasn't yet written.
    // therefore, we expect to have no value yet for formula output (0 is default from dummy construction)
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 0);

    // write to main value and check result
    accMathWrite = 3;
    accMathWrite.write();
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)pushPar + (int)accMathWrite);

    // write to push-parameter and check result
    // note, it's a new feature that result is completely written when write() returns.
    pushPar = 4;
    pushPar.write();
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)pushPar + (int)accMathWrite);

    // check that MathPlugin waits on all initial values.
    logicalDevice.close();
    targetDevice.open(); // open again since low-level device was closed by LNM
    accTarget = 0;       // reset result in dummy
    accTarget.write();
    pollPar.write(); // restore value in case lost during reopen (actually not required with current dummy impl)
    logicalDevice.open();
    logicalDevice.activateAsyncRead();

    accMathWrite = 5;
    accMathWrite.write();
    // check that MathPlugin did not yet write to the device - it must wait on push-parameter value
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 0);

    pushPar.write();
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)pushPar + (int)accMathWrite);

    // user expectation will be that write-behavior does not depend on when we call activateAsyncRead,
    // so test that update is retrieved even if it's called last
    logicalDevice.close();
    logicalDevice.open();
    accTarget = 0;
    accTarget.write();
    pollPar.write();
    accMathWrite = 7;
    accMathWrite.write();
    pushPar = 6;
    pushPar.write();
    logicalDevice.activateAsyncRead();
    accTarget.readLatest();
    BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)pushPar + (int)accMathWrite);
  }
  // regression test for bug https://redmine.msktools.desy.de/issues/11506
  // (math plugin + push-parameter + shm has resource cleanup problem)
  BOOST_CHECK(DummyForCleanupCheck::cleanupCalled);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
