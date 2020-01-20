#define BOOST_TEST_MODULE testDeviceExceptionFlagPropagation

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <ChimeraTK/DummyRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ExceptionDevice.h"
#include "PeriodicTrigger.h"
#include "TestFacility.h"
#include "VariableGroup.h"

namespace ctk = ChimeraTK;

constexpr char ExceptionDummyCDD1[] = "(ExceptionDummy:1?map=test3.map)";

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                      \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(!(condition)) {                                                                                              \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) break;                                                                                       \
      usleep(1000);                                                                                                    \
    }                                                                                                                  \
  }

struct TestApplication : ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {}

  struct : ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    struct : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarOutput<uint64_t> tick{this, "tick", "", ""};
    } name{this, "name", ""};

    void mainLoop() override {}
  } name{this, "name", ""};

  struct : ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    mutable int readMode{0};

    struct : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<uint64_t> tick{this, "tick", "", ""};
      ctk::ScalarPollInput<int> read{this, "readBack", "", ""};
      ctk::ScalarOutput<int> set{this, "actuator", "", ""};
    } vars{this, "vars", "", ctk::HierarchyModifier::hideThis};

    void mainLoop() override {
      while(true) {
        vars.tick.read();
        switch(readMode) {
          case 0:
            vars.read.readNonBlocking();
            break;
          case 1:
            vars.read.readLatest();
            break;
          case 2:
            vars.read.readAsync().wait();
            break;
          case 3:
            vars.read.read();
            break;
          case 5:
            vars.set.write();
            break;
          case 6:
            vars.set.write();
            break;
          default:
            break;
        }
      }
    }

  } module{this, "module", ""};

  ctk::PeriodicTrigger trigger{this, "trigger", ""};

  ctk::DeviceModule dev{this, ExceptionDummyCDD1};
  ctk::ControlSystemModule cs;
};

BOOST_AUTO_TEST_CASE(testDirectConnectOpen) {
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));

  app.dev("/MyModule/readBack", typeid(int), 1) >> app.module.vars.read;
  app.module.vars.set >> app.dev("/MyModule/actuator", typeid(int), 1);
  app.name.name.tick >> app.module.vars.tick;

  // Open

  dummyBackend1->throwExceptionOpen = true;
  ctk::TestFacility test(false);
  CHECK_TIMEOUT(app.module.vars.read.dataValidity() == ctk::DataValidity::ok, 1000);

  app.run();

  // Advance through all non-blocking read methods - write will block when open fails
  while(app.module.readMode < 3) {
    // Check
    app.name.name.tick.write();
    std::cout << "Checking read mode " << app.module.readMode << "\n";
    CHECK_TIMEOUT(app.module.vars.read.dataValidity() == ctk::DataValidity::faulty, 1000);

    // Reset data validity
    app.module.vars.read.setDataValidity();
    CHECK_TIMEOUT(app.module.vars.read.dataValidity() == ctk::DataValidity::ok, 1000);

    // advance to the next read
    app.module.readMode++;
  }

  // Unblock last read()
  dummyBackend1->throwExceptionOpen = false;
  app.module.vars.set.write();
}

BOOST_AUTO_TEST_CASE(testDirectConnectRead) {
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));

  app.dev("/MyModule/readBack", typeid(int), 1) >> app.module.vars.read;
  app.module.vars.set >> app.dev("/MyModule/actuator", typeid(int), 1);
  app.trigger.tick >> app.module.vars.tick;

  ctk::TestFacility test(true);
  test.runApplication();

  // Advance through all non-blocking read methods
  while(app.module.readMode < 4) {
    // Check
    app.trigger.sendTrigger();
    test.stepApplication();
    BOOST_CHECK(app.module.vars.read.dataValidity() == ctk::DataValidity::ok);

    // Check
    std::cout << "Checking read mode " << app.module.readMode << "\n";
    dummyBackend1->throwExceptionRead = true;
    app.trigger.sendTrigger();
    test.stepApplication();
    BOOST_CHECK(app.module.vars.read.dataValidity() == ctk::DataValidity::faulty);

    // advance to the next read
    dummyBackend1->throwExceptionRead = false;
    app.module.readMode++;

    // Skip readAsync(). See https://github.com/ChimeraTK/ApplicationCore/issues/48
    if(app.module.readMode == 2) app.module.readMode++;
  }
}

BOOST_AUTO_TEST_CASE(testDirectConnectWrite) {
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));

  app.dev("/MyModule/readBack", typeid(int), 1) >> app.module.vars.read;
  app.module.vars.set >> app.dev("/MyModule/actuator", typeid(int), 1);
  app.module.readMode = 5;
  app.trigger.tick >> app.module.vars.tick;

  ctk::TestFacility test(true);
  test.runApplication();

  // Advance through all non-blocking read methods
  while(app.module.readMode < 7) {
    // Check
    app.trigger.sendTrigger();
    test.stepApplication();
    BOOST_CHECK(app.module.vars.set.dataValidity() == ctk::DataValidity::ok);

    // Check
    dummyBackend1->throwExceptionWrite = true;
    app.trigger.sendTrigger();
    test.stepApplication();
    // write operations failing does not invalidate data
    BOOST_CHECK(app.module.vars.set.dataValidity() == ctk::DataValidity::ok);

    // advance to the next read
    dummyBackend1->throwExceptionWrite = false;
    app.module.readMode++;
  }
}
