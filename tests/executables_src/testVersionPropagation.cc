#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "Flags.h"
#include "TestFacility.h"
#include "VariableGroup.h"
#include "check_timeout.h"
#include <ChimeraTK/VersionNumber.h>

#define BOOST_TEST_MODULE testVersionpropagation
#include <boost/test/included/unit_test.hpp>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/RegisterPath.h>
#include <future>

namespace ctk = ChimeraTK;

struct PollModule : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPollInput<int> pollInput{this, "REG1", "", "", {"DEVICE"}};

  void mainLoop() override {}
};

struct PushModule : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  struct : ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<int> pushInput{this, "PUSH_READ", "", ""};
  } reg1{this, "REG1", ""};

  void mainLoop() override {}
  //void mainLoop() override {
  //  while(true) {
  //    reg1.pushInput.read();
  //    std::cout << "PUSH_INPUT" << (int)reg1.pushInput << std::endl;
  //  }
  //}
};

struct DummyApplication : ctk::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  DummyApplication() : Application("DummyApplication") {}
  ~DummyApplication() { shutdown(); }

  PushModule pushModule{this, "", ""};
  PollModule pollModule{this, "", ""};

  ctk::ControlSystemModule cs;
  ctk::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    findTag("CS").connectTo(cs);
    findTag("DEVICE").connectTo(device);

    auto push_input = device("REG1/PUSH_READ", typeid(int), 1, ctk::UpdateMode::push);
    push_input >> pushModule.reg1.pushInput;
  }
};

struct Fixture {
  Fixture()
  : deviceBackend(boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(DummyApplication::ExceptionDummyCDD1))) {
    deviceBackend->open();
    testFacitiy.runApplication();

    exceptionDummyRegister.replace(application.device.device.getScalarRegisterAccessor<int>("/REG1"));
    status.replace(
        testFacitiy.getScalar<int>(ctk::RegisterPath("/Devices") / DummyApplication::ExceptionDummyCDD1 / "status"));

    //
    //  workaround to ensure we exit only after device setup is complete
    /************************************************************************************************/
    CHECK_TIMEOUT(
        [&]() {
          status.readLatest();
          return static_cast<int>(status);
        }() == 0,
        100000);
    /************************************************************************************************/
  }

  boost::shared_ptr<ctk::ExceptionDummy> deviceBackend;
  DummyApplication application;
  ctk::TestFacility testFacitiy{true};

  ctk::ScalarRegisterAccessor<int> status;
  ctk::ScalarRegisterAccessor<int> exceptionDummyRegister;
};

BOOST_AUTO_TEST_SUITE(versionPropagation)
BOOST_FIXTURE_TEST_CASE(versionPropagation_testPolledRead, Fixture) {
  std::cout << "versionPropagation_testPolledRead" << std::endl;
  auto& pollVariable = application.pollModule.pollInput;
  auto moduleVersion = application.pollModule.getCurrentVersionNumber();
  auto pollVariableVersion = pollVariable.getVersionNumber();

  pollVariable.read();

  BOOST_CHECK(pollVariable.getVersionNumber() > pollVariableVersion);
  BOOST_CHECK(moduleVersion == application.pollModule.getCurrentVersionNumber());
}

BOOST_FIXTURE_TEST_CASE(versionPropagation_testPolledReadNonBlocking, Fixture) {
  std::cout << "versionPropagation_testPolledReadNonBlocking" << std::endl;
  auto& pollVariable = application.pollModule.pollInput;
  auto moduleVersion = application.pollModule.getCurrentVersionNumber();
  auto pollVariableVersion = pollVariable.getVersionNumber();

  pollVariable.readNonBlocking();

  BOOST_CHECK(pollVariable.getVersionNumber() > pollVariableVersion);
  BOOST_CHECK(moduleVersion == application.pollModule.getCurrentVersionNumber());
}

BOOST_FIXTURE_TEST_CASE(versionPropagation_testPolledReadLatest, Fixture) {
  std::cout << "versionPropagation_testPolledReadLatest" << std::endl;
  auto& pollVariable = application.pollModule.pollInput;
  auto moduleVersion = application.pollModule.getCurrentVersionNumber();
  auto pollVariableVersion = pollVariable.getVersionNumber();

  pollVariable.readLatest();

  BOOST_CHECK(pollVariable.getVersionNumber() > pollVariableVersion);
  BOOST_CHECK(moduleVersion == application.pollModule.getCurrentVersionNumber());
}

BOOST_FIXTURE_TEST_CASE(versionPropagation_testPushTypeRead, Fixture) {
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

BOOST_FIXTURE_TEST_CASE(versionPropagation_testPushTypeReadNonBlocking, Fixture) {
  std::cout << "versionPropagation_testPushTypeReadNonBlocking" << std::endl;
  auto& pushInput = application.pushModule.reg1.pushInput;
  // Make sure we pop out any stray values in the pushInput before test start:
  CHECK_TIMEOUT(pushInput.readLatest() == false, 10000);

  auto pushInputVersionNumber = pushInput.getVersionNumber();

  // no version change on readNonBlocking false
  BOOST_CHECK_EQUAL(pushInput.readNonBlocking(), false);
  BOOST_CHECK(pushInputVersionNumber == pushInput.getVersionNumber());

  ctk::VersionNumber nextVersionNumber = {};
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), nextVersionNumber);
  BOOST_CHECK_EQUAL(pushInput.readNonBlocking(), true);
  BOOST_CHECK(nextVersionNumber == pushInput.getVersionNumber());
  BOOST_CHECK(nextVersionNumber == application.pushModule.getCurrentVersionNumber());
}

BOOST_FIXTURE_TEST_CASE(versionPropagation_testPushTypeReadLatest, Fixture) {
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
  BOOST_CHECK_EQUAL(pushInput.readLatest(), true);
  BOOST_CHECK(nextVersionNumber == pushInput.getVersionNumber());
  BOOST_CHECK(nextVersionNumber == application.pushModule.getCurrentVersionNumber());
}
BOOST_AUTO_TEST_SUITE_END()
