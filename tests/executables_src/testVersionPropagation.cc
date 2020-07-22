#define BOOST_TEST_MODULE testVersionpropagation

#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "Flags.h"
#include "TestFacility.h"
#include "VariableGroup.h"
#include "check_timeout.h"
#include "fixtures.h"

#include <boost/test/included/unit_test.hpp>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/RegisterPath.h>
#include <future>
#include <ChimeraTK/VersionNumber.h>

namespace ctk = ChimeraTK;
using Fixture_testFacilityEnabled = fixture_with_poll_and_push_input<true>;


BOOST_FIXTURE_TEST_SUITE(versionPropagation, Fixture_testFacilityEnabled)

BOOST_AUTO_TEST_CASE(versionPropagation_testPolledRead) {
  std::cout << "versionPropagation_testPolledRead" << std::endl;
  auto& pollVariable = application.pollModule.pollInput;
  auto moduleVersion = application.pollModule.getCurrentVersionNumber();
  auto pollVariableVersion = pollVariable.getVersionNumber();

  pollVariable.read();

  BOOST_CHECK(pollVariable.getVersionNumber() > pollVariableVersion);
  BOOST_CHECK(moduleVersion == application.pollModule.getCurrentVersionNumber());
}

BOOST_AUTO_TEST_CASE(versionPropagation_testPolledReadNonBlocking) {
  std::cout << "versionPropagation_testPolledReadNonBlocking" << std::endl;
  auto& pollVariable = application.pollModule.pollInput;
  auto moduleVersion = application.pollModule.getCurrentVersionNumber();
  auto pollVariableVersion = pollVariable.getVersionNumber();

  pollVariable.readNonBlocking();

  BOOST_CHECK(pollVariable.getVersionNumber() > pollVariableVersion);
  BOOST_CHECK(moduleVersion == application.pollModule.getCurrentVersionNumber());
}

BOOST_AUTO_TEST_CASE(versionPropagation_testPolledReadLatest) {
  std::cout << "versionPropagation_testPolledReadLatest" << std::endl;
  auto& pollVariable = application.pollModule.pollInput;
  auto moduleVersion = application.pollModule.getCurrentVersionNumber();
  auto pollVariableVersion = pollVariable.getVersionNumber();

  pollVariable.readLatest();

  BOOST_CHECK(pollVariable.getVersionNumber() > pollVariableVersion);
  BOOST_CHECK(moduleVersion == application.pollModule.getCurrentVersionNumber());
}

BOOST_AUTO_TEST_CASE(versionPropagation_testPushTypeRead) {
  std::cout << "versionPropagation_testPushTypeRead" << std::endl;
  auto& pushInput = application.pushModule.reg1.pushInput;
  // Make sure we pop out any stray values in the pushInput before test start:
  CHECK_TIMEOUT(pushInput.readLatest() == false, 10000);

  ctk::VersionNumber nextVersionNumber = {};
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), nextVersionNumber);
  pushInput.read();
  BOOST_CHECK(pushInput.getVersionNumber() == nextVersionNumber);
  BOOST_CHECK(application.pushModule.getCurrentVersionNumber() == nextVersionNumber);
}

BOOST_AUTO_TEST_CASE(versionPropagation_testPushTypeReadNonBlocking) {
  std::cout << "versionPropagation_testPushTypeReadNonBlocking" << std::endl;
  auto& pushInput = application.pushModule.reg1.pushInput;
  // Make sure we pop out any stray values in the pushInput before test start:
  CHECK_TIMEOUT(pushInput.readLatest() == false, 10000);

  auto pushInputVersionNumber = pushInput.getVersionNumber();

  // no version change on readNonBlocking false
  BOOST_CHECK_EQUAL(pushInput.readNonBlocking(), false);
  BOOST_CHECK(pushInputVersionNumber == pushInput.getVersionNumber());

  ctk::VersionNumber nextVersionNumber = {};
  auto moduleVersion = application.pushModule.getCurrentVersionNumber();
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), nextVersionNumber);
  BOOST_CHECK_EQUAL(pushInput.readNonBlocking(), true);
  BOOST_CHECK(nextVersionNumber == pushInput.getVersionNumber());

  // readNonBlocking will not propagete the version to the module
  BOOST_CHECK(moduleVersion == application.pushModule.getCurrentVersionNumber());
}

BOOST_AUTO_TEST_CASE(versionPropagation_testPushTypeReadLatest) {
  std::cout << "versionPropagation_testPushTypeReadLatest" << std::endl;
  auto& pushInput = application.pushModule.reg1.pushInput;
  // Make sure we pop out any stray values in the pushInput before test start:
  CHECK_TIMEOUT(pushInput.readLatest() == false, 10000);

  auto pushInputVersionNumber = pushInput.getVersionNumber();

  // no version change on readNonBlocking false
  BOOST_CHECK_EQUAL(pushInput.readLatest(), false);
  BOOST_CHECK(pushInputVersionNumber == pushInput.getVersionNumber());

  ctk::VersionNumber nextVersionNumber = {};
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), nextVersionNumber);
  auto moduleVersion = application.pushModule.getCurrentVersionNumber();
  BOOST_CHECK_EQUAL(pushInput.readLatest(), true);
  BOOST_CHECK(nextVersionNumber == pushInput.getVersionNumber());

  // readLatest will not propagete the version to the module
  BOOST_CHECK(moduleVersion == application.pushModule.getCurrentVersionNumber());
}

BOOST_AUTO_TEST_SUITE_END()
