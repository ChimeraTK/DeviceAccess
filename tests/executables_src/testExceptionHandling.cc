#define BOOST_TEST_MODULE testExceptionHandling

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <cstring>
#include <future>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/DummyRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "check_timeout.h"
#include "fixtures.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;
using Fixture = fixture_with_poll_and_push_input<false>;

/*
 * This test suite checks behavior on a device related runtime error.
 */
BOOST_AUTO_TEST_SUITE(runtimeErrorHandling)

/*
  * Verify the framework creates fault indicator process variables for a device.
  *
  * These are mapped on the control system as:
  *   - /Devices/<device_alias or cdd>/status
  *   - /Devices/<device_alias or cdd>/message
  *
  * A runtime errror on <device_alias> changes status to 1, with a non empty message
  * string.
  *
  * \anchor testExceptionHandling_b_2_1 \ref exceptionHandling_b_2_1 "B.2.1"
  */
BOOST_FIXTURE_TEST_CASE(runtimeErrorHandling_testFaultReporting, Fixture) {
  std::cout << "runtimeErrorHandling_testFaultReporting" << std::endl;

  // These are instantiated in the fixture:
  // status -> /Devices/(ExceptionDummy:1?map=test.map)/status
  // message -> /Devices/(ExceptionDummy:1?map=test.map)/message

  BOOST_CHECK_EQUAL(status, 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(message), "");

  deviceBackend->throwExceptionRead = true;
  application.pollModule.pollInput.read(); // causes device exception

  CHECK_TIMEOUT(status.readNonBlocking() == true, 10000);
  CHECK_TIMEOUT(message.readNonBlocking() == true, 10000);
  BOOST_CHECK_EQUAL(status, 1);
  BOOST_CHECK(static_cast<std::string>(message) != "");

  deviceBackend->throwExceptionRead = false;

  CHECK_TIMEOUT(status.readNonBlocking() == true, 10000);
  CHECK_TIMEOUT(message.readNonBlocking() == true, 10000);
  BOOST_CHECK_EQUAL(status, 0);
  BOOST_CHECK(static_cast<std::string>(message) == "");
}

/*
 * Read from a device in error, using pollType Process Variable.
 * 
 * Expected behavior:

 *   - Calls to read are skipped till the device recovers.
 *
 *   - The first skipped read request:
 *     - Sets the process variable data validity flag to faulty
 *     - Generates a new version number. 
 *
  * \anchor testExceptionHandling_b_2_2_3 \ref exceptionHandling_b_2_2_3 "B.2.2.3"
 */
BOOST_FIXTURE_TEST_CASE(runtimeErrorHandling_testPolledRead, Fixture) {
  std::cout << "runtimeErrorHandling_testPolledRead" << std::endl;

  // initialize to known value in deviceBackend register
  exceptionDummyRegister = 100;
  exceptionDummyRegister.write();

  // verify normal operation
  /************************************************************************************************/
  pollVariable.read();
  auto versionNumberBeforeRuntimeError = pollVariable.getVersionNumber();

  BOOST_CHECK_EQUAL(pollVariable, 100);
  BOOST_CHECK(pollVariable.dataValidity() == ctk::DataValidity::ok);
  /************************************************************************************************/

  // Behavior on Runtime error on device:
  /************************************************************************************************/
  exceptionDummyRegister = 10;
  exceptionDummyRegister.write();

  deviceBackend->throwExceptionRead = true;
  pollVariable.read();

  // Proceed only after device is gone down.
  CHECK_TIMEOUT(
      [&]() {
        status.readLatest();
        return static_cast<int>(status);
      }() == 1,
      10000);

  pollVariable.read();
  auto versionNumberOnRuntimeError = pollVariable.getVersionNumber();

  BOOST_CHECK_EQUAL(pollVariable, 100);
  BOOST_CHECK(pollVariable.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(versionNumberOnRuntimeError > versionNumberBeforeRuntimeError);
  /************************************************************************************************/

  // Behavior on device recovery
  /************************************************************************************************/
  deviceBackend->throwExceptionRead = false;
  pollVariable.read();
  // workaround: wait till device module recovey completes; assumption: status variable == 0 =>
  // device recovered.
  CHECK_TIMEOUT(
      [&]() {
        status.readLatest();
        return static_cast<int>(status);
      }() == 0,
      10000);

  pollVariable.read();
  auto versionNumberAfterRecovery = pollVariable.getVersionNumber();

  BOOST_REQUIRE_EQUAL(pollVariable, 10);
  BOOST_CHECK(pollVariable.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(versionNumberAfterRecovery > versionNumberOnRuntimeError);
  /************************************************************************************************/
}

/**
 * Read from a device in error using a pushType Process Variable (PV)
 *
 * Expected Behavior:
 *
 *  - First call on pushType PV read is skipped. This:
 *    - Generates a new version number
 *    - Sets PV data validity flag to faulty.
 *
 *  - The subsequent call on read blocks till device recovery.
 *
 * \anchor testExceptionHandling_b_2_2_4_a \ref exceptionHandling_b_2_2_4 "B.2.2.4"
 */
BOOST_FIXTURE_TEST_CASE(runtimeErrorHandling_testPushTypeRead, Fixture) {
  std::cout << "runtimeErrorHandling_testPushTypeRead" << std::endl;

  exceptionDummyRegister = 100;
  exceptionDummyRegister.write();
  ctk::VersionNumber versionBeforeRuntimeError = {};
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), versionBeforeRuntimeError);

  pushVariable.read();
  BOOST_CHECK_EQUAL(pushVariable, 100);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(pushVariable.getVersionNumber() == versionBeforeRuntimeError);

  // Behavior on Runtime error on device:
  //  - push input read is skipped.
  /************************************************************************************************/
  exceptionDummyRegister = 10;
  exceptionDummyRegister.write();

  deviceBackend->throwExceptionRead = true;
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"));

  pushVariable.read();
  BOOST_CHECK_EQUAL(pushVariable, 100);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(pushVariable.getVersionNumber() > versionBeforeRuntimeError);

  //  - subsequent push input reads should be frozen
  /************************************************************************************************/
  auto f = std::async(std::launch::async, [&]() { pushVariable.read(); });
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"));

  // FIXME: is there a better way to confirm read is frozen?
  auto future_status = f.wait_for(std::chrono::seconds(1));
  BOOST_CHECK(future_status == std::future_status::timeout);

  //  - Remove Runtime Error
  /************************************************************************************************/
  deviceBackend->throwExceptionRead = false;
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"));

  BOOST_CHECK(f.wait_for(std::chrono::seconds(10)) == std::future_status::ready);
  BOOST_CHECK_EQUAL(pushVariable, 10);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::ok);
}

/*
 * Test reaNonblocking from a device in error using a push type Process
 * Variable
 *
 * Expected Behavior:
 *
 *  - First call to readNonBlocking after device error:
 *    - Returns true.
 *    - Generates a new version number.
 *    - Sets Process Variable data validity to faulty
 *
 *  - Subsequent calls till recovery are skipped and return false.
 *
 * \anchor testExceptionHandling_b_2_2_4_b \ref exceptionHandling_b_2_2_4 "B.2.2.4"
 */
BOOST_FIXTURE_TEST_CASE(runtimeErrorHandling_testPushTypeReadNonBlocking, Fixture) {
  std::cout << "runtimeErrorHandling_testPushTypeReadNonBlocking" << std::endl;

  // precondition: no pending data to be read on the push type variable
  BOOST_CHECK_EQUAL(pushVariable.readNonBlocking(), false);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::ok);

  //  On runtime error
  //    - return true.
  //    - generates a new version number.
  /************************************************************************************************/
  exceptionDummyRegister = 100;
  exceptionDummyRegister.write();
  ctk::VersionNumber version = {};

  deviceBackend->throwExceptionRead = true;
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), version);

  CHECK_TIMEOUT(pushVariable.readNonBlocking() == true, 10000);
  BOOST_CHECK_NE(pushVariable, 100);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::faulty);
  auto versionNumberOnRuntimeError = pushVariable.getVersionNumber();
  BOOST_CHECK(versionNumberOnRuntimeError > version);

  //  subsequent calls to readNonBlocking on runtime error are skipped.
  /************************************************************************************************/
  BOOST_CHECK_EQUAL(pushVariable.readNonBlocking(), false);
  BOOST_CHECK(versionNumberOnRuntimeError == pushVariable.getVersionNumber());
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::faulty);

  // On recovery
  /************************************************************************************************/
  deviceBackend->throwExceptionRead = false;
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"));

  CHECK_TIMEOUT(pushVariable.readNonBlocking() == true, 10000);
  BOOST_CHECK_EQUAL(pushVariable, 100);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(pushVariable.getVersionNumber() > versionNumberOnRuntimeError);
}

/*
 * Test readLatest from a device in error using a push type Process
 * Variable
 *
 * Expected Behavior:
 *
 *  - First call to readLatest after device error:
 *    - Returns true.
 *    - Generates a new version number.
 *    - Sets Process Variable data validity to faulty
 *
 *  - Subsequent calls till recovery are skipped and return false.
 *
 * \anchor testExceptionHandling_b_2_2_4_c \ref exceptionHandling_b_2_2_4 "B.2.2.4"
 */
BOOST_FIXTURE_TEST_CASE(runtimeErrorHandling_testPushTypeReadLatest, Fixture) {
  std::cout << "runtimeErrorHandling_testPushTypeReadLatest" << std::endl;

  // precondition: no pending data to be read on the push type 
  BOOST_CHECK_EQUAL(pushVariable.readLatest(), false);
  BOOST_CHECK_EQUAL(pushVariable, 0);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::ok);

  //  On runtime error
  //    - return true.
  //    - generates a new version number.
  /************************************************************************************************/
  exceptionDummyRegister = 100;
  exceptionDummyRegister.write();
  ctk::VersionNumber version = {};

  deviceBackend->throwExceptionRead = true;
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"), version);

  CHECK_TIMEOUT(pushVariable.readLatest() == true, 10000);
  BOOST_CHECK_NE(pushVariable, 100);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::faulty);
  auto versionNumberOnRuntimeError = pushVariable.getVersionNumber();
  BOOST_CHECK(versionNumberOnRuntimeError > version);

  //  subsequent calls to readLatest on runtime error are skipped.
  /************************************************************************************************/
  BOOST_CHECK_EQUAL(pushVariable.readLatest(), false);
  BOOST_CHECK(versionNumberOnRuntimeError == pushVariable.getVersionNumber());
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::faulty);

  // On recovery
  /************************************************************************************************/
  deviceBackend->throwExceptionRead = false;
  deviceBackend->triggerPush(ctk::RegisterPath("REG1/PUSH_READ"));

  CHECK_TIMEOUT(pushVariable.readLatest() == true, 10000);
  BOOST_CHECK_EQUAL(pushVariable, 100);
  BOOST_CHECK(pushVariable.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(pushVariable.getVersionNumber() > versionNumberOnRuntimeError);
}

BOOST_AUTO_TEST_CASE(testWrite) {}

BOOST_AUTO_TEST_SUITE_END()

constexpr char ExceptionDummyCDD1[] = "(ExceptionDummy:1?map=test3.map)";
constexpr char ExceptionDummyCDD2[] = "(ExceptionDummy:2?map=test3.map)";
constexpr char ExceptionDummyCDD3[] = "(ExceptionDummy:3?map=test4.map)";

/* dummy application */

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests

  ctk::DeviceModule dev1{this, ExceptionDummyCDD1};
  ctk::DeviceModule dev2{this, ExceptionDummyCDD2};
  ctk::ControlSystemModule cs;
};

struct OutputModule : public ctk::ApplicationModule {
  OutputModule(EntityOwner* owner, const std::string& name, const std::string& description,
      ctk::HierarchyModifier hierarchyModifier = ctk::HierarchyModifier::none,
      const std::unordered_set<std::string>& tags = {})
  : ApplicationModule(owner, name, description, hierarchyModifier, tags), mainLoopStarted(2) {}

  ctk::ScalarPushInput<int32_t> trigger{this, "trigger", "", "I wait for this to start."};
  ctk::ScalarOutput<int32_t> actuator{this, "actuator", "", "This is where I write to."};

  void mainLoop() override {
    mainLoopStarted.wait();

    trigger.read();
    actuator = int32_t(trigger);
    actuator.write();
  }

  // We do not use testable mode for this test, so we need this barrier to synchronise to the beginning of the
  // mainLoop(). This is required to make sure the initial value propagation is done.
  // execute this right after the Application::run():
  //   app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered
  boost::barrier mainLoopStarted;
};

struct InputModule : public ctk::ApplicationModule {
  InputModule(EntityOwner* owner, const std::string& name, const std::string& description,
      ctk::HierarchyModifier hierarchyModifier = ctk::HierarchyModifier::none,
      const std::unordered_set<std::string>& tags = {})
  : ApplicationModule(owner, name, description, hierarchyModifier, tags), mainLoopStarted(2) {}

  ctk::ScalarPushInput<int32_t> trigger{this, "trigger", "", "I wait for this to start."};
  ctk::ScalarPollInput<int32_t> readback{this, "readback", "", "Just going to read something."};

  void mainLoop() override {
    mainLoopStarted.wait();

    trigger.read();
    readback.read();
    // I am not doing anything with the read values, but still a useful test (we do not get here anyway)
  }

  // We do not use testable mode for this test, so we need this barrier to synchronise to the beginning of the
  // mainLoop(). This is required to make sure the initial value propagation is done.
  // execute this right after the Application::run():
  //   app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered
  boost::barrier mainLoopStarted;
};

struct RealisticModule : public ctk::ApplicationModule {
  RealisticModule(EntityOwner* owner, const std::string& name, const std::string& description,
      ctk::HierarchyModifier hierarchyModifier = ctk::HierarchyModifier::none,
      const std::unordered_set<std::string>& tags = {})
  : ApplicationModule(owner, name, description, hierarchyModifier, tags), mainLoopStarted(2) {}

  ctk::ScalarPushInput<int32_t> reg1{this, "REG1", "", "misused as input"};
  ctk::ScalarPollInput<int32_t> reg2{this, "REG2", "", "also no input..."};
  ctk::ScalarOutput<int32_t> reg3{this, "REG3", "", "my output"};

  void mainLoop() override {
    mainLoopStarted.wait();

    reg1.read();
    reg2.readLatest();

    reg3 = reg1 * reg2;
    reg3.write();
  }

  // We do not use testable mode for this test, so we need this barrier to synchronise to the beginning of the
  // mainLoop(). This is required to make sure the initial value propagation is done.
  // execute this right after the Application::run():
  //   app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered
  boost::barrier mainLoopStarted;
};

// A more compicated scenario with module that have blocking reads and writes, fans that connect to the device and the CS, and direct connection device/CS only without fans.
struct TestApplication2 : public ctk::Application {
  TestApplication2() : Application("testSuite") {}
  ~TestApplication2() { shutdown(); }

  void defineConnections() {
    // let's do some manual cabling here....
    // A module that is only writin to a device such that no fan is involved
    cs("triggerActuator") >> outputModule("trigger");
    outputModule("actuator") >> dev1["MyModule"]("actuator");

    cs("triggerReadback") >> inputModule("trigger");
    dev1["MyModule"]("readBack") >> inputModule("readback");

    dev2.connectTo(cs["Device2"], cs("trigger2", typeid(int), 1));

    // the most realistic part: everything cabled everywhere with fans
    // first cable the module to the device. This determines the direction of the variables
    // FIXME: This does not work, don't know why.
    //dev3["MODULE"]("REG1")[ cs("triggerRealistic",typeid(int), 1) ] >> realisticModule("REG1");

    // This is not what I wanted. I wanted a triggered network for Reg1 and Reg2
    realisticModule("REG3") >> dev3["MODULE"]("REG3"); // for the direction
    dev3.connectTo(cs["Device3"], cs("triggerRealistic", typeid(int), 1));
    realisticModule.connectTo(cs["Device3"]["MODULE"]);
  }

  OutputModule outputModule{this, "outputModule", "The output module"};
  InputModule inputModule{this, "inputModule", "The input module"};

  RealisticModule realisticModule{this, "realisticModule", "The most realistic module"};

  ctk::DeviceModule dev1{this, ExceptionDummyCDD1};
  ctk::DeviceModule dev2{this, ExceptionDummyCDD2};
  ctk::DeviceModule dev3{this, ExceptionDummyCDD3};
  ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testExceptionHandlingRead) {
  std::cout << "testExceptionHandlingRead" << std::endl;
  TestApplication app;
  boost::shared_ptr<ctk::ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ctk::ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ctk::Device dev1(ExceptionDummyCDD1);
  ctk::Device dev2(ExceptionDummyCDD2);

  // Connect the whole devices into the control system, and use the control system variable /trigger as trigger for
  // both devices. The variable becomes a control system to application variable and writing to it through the test
  // facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));

  // Do not enable testable mode. The testable mode would block in the wrong place, as the trigger for reading variables
  // of a device in the error state is not being processed until the error state is cleared. We want to test that the
  // second device still works while the first device is in error state, which would be impossible with testable mode
  // enabled. As a consequence, our test logic has to work with timeouts (CHECK_TIMEOUT) etc. instead of the
  // deterministic stepApplication().
  ctk::TestFacility test(false);
  test.runApplication();

  auto message1 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD1 + "/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  auto readback1 = test.getScalar<int>("/Device1/MyModule/readBack");
  auto message2 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD2 + "/message");
  auto status2 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD2 + "/status");
  auto readback2 = test.getScalar<int>("/Device2/MyModule/readBack");

  auto trigger = test.getScalar<int>("trigger");

  // we do not use testable mode, so we need to read the initial values at CS ourself where present
  readback1.read();
  readback2.read();

  dev1.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 42);
  dev2.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 52);
  int readback1_expected = 42;

  // initially, devices are not opened but errors should be cleared once they are opened
  trigger.write();

  do {
    message1.readLatest();
    status1.readLatest();
  } while(status1 != 0 || std::string(message1) != "");
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());

  do {
    message2.readLatest();
    status2.readLatest();
  } while(status2 != 0 || std::string(message2) != "");
  BOOST_CHECK(!message2.readLatest());
  BOOST_CHECK(!status2.readLatest());

  CHECK_TIMEOUT(readback1.readLatest(), 10000);
  CHECK_TIMEOUT(readback2.readLatest(), 10000);
  BOOST_CHECK_EQUAL(readback1, readback1_expected);
  BOOST_CHECK_EQUAL(readback2, 52);

  // repeat test a couple of times to make sure it works not only once
  for(int i = 0; i < 3; ++i) {
    // enable exception throwing in test device 1
    dev1.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 10 + i);
    dev2.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 20 + i);
    dummyBackend1->throwExceptionRead = true;
    trigger.write();
    CHECK_TIMEOUT(message1.readLatest(), 10000);
    CHECK_TIMEOUT(status1.readLatest(), 10000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    BOOST_CHECK(readback1.readNonBlocking());                                 // we have been signalized new data
    BOOST_CHECK(readback1.dataValidity() == ChimeraTK::DataValidity::faulty); // But the fault flag should be set
    // the second device must still be functional
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(readback2.readNonBlocking(), 10000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 20 + i);

    // even with device 1 failing the trigger produces "new" data: The time stamp changes, but the data content does not, and it is flagged as invalid
    // device 2 is working normaly
    dev2.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 120 + i);
    trigger.write();
    readback1.read();
    BOOST_CHECK_EQUAL(readback1, readback1_expected);                         // The value has not changed
    BOOST_CHECK(readback1.dataValidity() == ChimeraTK::DataValidity::faulty); // But the fault flag should still be set
    CHECK_TIMEOUT(readback2.readNonBlocking(), 10000);                        // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 120 + i);

    // Now "cure" the device problem
    dummyBackend1->throwExceptionRead = false;
    // we have to wait until the device has recovered. Otherwise the writing will throw.
    CHECK_TIMEOUT(status1.readLatest(), 10000);
    BOOST_CHECK_EQUAL(status1, 0);

    dev1.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 30 + i);
    dev2.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 40 + i);
    trigger.write();
    message1.read();
    readback1.read();
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(readback1,
        30 + i); // the "20+i" is never seen because there was a new vaulue before the next trigger after the recovery
    readback1_expected = 30 + i; // remember the last good value for the next iteration
    BOOST_CHECK(readback1.dataValidity() == ChimeraTK::DataValidity::ok); // The fault flag should have been cleared
    // device2
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(readback2.readNonBlocking(), 10000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 40 + i);
  }
  // FIXME: This test only works for poll-type variables. We also have to test with push type.
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandlingWrite) {
  std::cout << "testExceptionHandlingWrite" << std::endl;
  TestApplication app;
  boost::shared_ptr<ctk::ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ctk::ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ctk::Device dev1(ExceptionDummyCDD1);
  ctk::Device dev2(ExceptionDummyCDD2);

  // Connect the whole devices into the control system, and use the control system variable /trigger as trigger for
  // both devices. The variable becomes a control system to application variable and writing to it through the test
  // facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));

  // Do not enable testable mode. The testable mode would block in the wrong place, as the trigger for reading variables
  // of a device in the error state is not being processed until the error state is cleared. We want to test that the
  // second device still works while the first device is in error state, which would be impossible with testable mode
  // enabled. As a consequence, our test logic has to work with timeouts (CHECK_TIMEOUT) etc. instead of the
  // deterministic stepApplication().
  ctk::TestFacility test(false);
  test.runApplication();

  auto message1 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD1 + "/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  auto actuator1 = test.getScalar<int>("/Device1/MyModule/actuator");
  auto message2 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD2 + "/message");
  auto status2 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD2 + "/status");
  auto actuator2 = test.getScalar<int>("/Device2/MyModule/actuator");

  auto trigger = test.getScalar<int>("trigger");

  // initially, devices are not opened but errors should be cleared once they are opened
  do {
    message1.readLatest();
    status1.readLatest();
  } while(status1 != 0 || std::string(message1) != "");
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());

  do {
    message2.readLatest();
    status2.readLatest();
  } while(status2 != 0 || std::string(message2) != "");
  BOOST_CHECK(!message2.readLatest());
  BOOST_CHECK(!status2.readLatest());

  actuator1 = 29;
  actuator1.write();
  actuator2 = 39;
  actuator2.write();
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());
  CHECK_TIMEOUT(dev1.read<int>("MyModule/actuator") == 29, 10000);
  CHECK_TIMEOUT(dev2.read<int>("MyModule/actuator") == 39, 10000);
  BOOST_CHECK(static_cast<std::string>(message1) == "");
  BOOST_CHECK(status1 == 0);

  // repeat test a couple of times to make sure it works not only once
  for(int i = 0; i < 3; ++i) {
    // enable exception throwing in test device 1
    dummyBackend1->throwExceptionWrite = true;
    actuator1 = 30 + i;
    actuator1.write();
    actuator2 = 40 + i;
    actuator2.write();
    CHECK_TIMEOUT(message1.readLatest(), 10000);
    CHECK_TIMEOUT(status1.readLatest(), 10000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    usleep(10000); // 10ms wait time so potential wrong values could have propagated
    // while the device is broken none of its accessors work. We have to use the dummy backend directly to look into its data buffer.
    auto actuatorDummyRaw = dummyBackend1->getRawAccessor("MyModule", "actuator");
    {
      auto bufferLock = actuatorDummyRaw.getBufferLock();
      BOOST_CHECK(actuatorDummyRaw == int(30 + i - 1)); // write not done for broken device
    }
    // the second device must still be functional
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(dev2.read<int>("MyModule/actuator") == int(40 + i), 10000); // device 2 still works

    // even with device 1 failing the second one must process the data, so send a new data before fixing dev1
    actuator2 = 120 + i;
    actuator2.write();
    CHECK_TIMEOUT(dev2.read<int>("MyModule/actuator") == int(120 + i), 10000); // device 2 still works
    {
      auto bufferLock = actuatorDummyRaw.getBufferLock();
      BOOST_CHECK(actuatorDummyRaw == int(30 + i - 1)); // device 1 is still broken and has not seen the new value yet
    }

    // Now "cure" the device problem
    dummyBackend1->throwExceptionWrite = false;
    CHECK_TIMEOUT(message1.readLatest(), 10000);
    CHECK_TIMEOUT(status1.readLatest(), 10000);
    CHECK_TIMEOUT(dev1.read<int>("MyModule/actuator") == int(30 + i), 10000); // write is now complete
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(status1, 0);
  }
}
/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandlingOpen) {
  std::cout << "testExceptionHandlingOpen" << std::endl;
  TestApplication app;
  boost::shared_ptr<ctk::ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ctk::ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ctk::Device dev1(ExceptionDummyCDD1);
  ctk::Device dev2(ExceptionDummyCDD2);
  dev1.open();
  dev2.open();
  dev1.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 100);
  dev2.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 110);
  dev1.close();
  dev2.close();

  // Connect the whole devices into the control system, and use the control system variable /trigger as trigger for
  // both devices. The variable becomes a control system to application variable and writing to it through the test
  // facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));

  // Do not enable testable mode. The testable mode would block in the wrong place, as the trigger for reading variables
  // of a device in the error state is not being processed until the error state is cleared. We want to test that the
  // second device still works while the first device is in error state, which would be impossible with testable mode
  // enabled. As a consequence, our test logic has to work with timeouts (CHECK_TIMEOUT) etc. instead of the
  // deterministic stepApplication().
  ctk::TestFacility test(false);
  dummyBackend1->throwExceptionOpen = true;
  app.run(); // don't use TestFacility::runApplication() here as it blocks until all devices are open...

  auto message1 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD1 + "/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  auto readback1 = test.getScalar<int>("/Device1/MyModule/readBack");
  auto message2 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD2 + "/message");
  auto status2 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD2 + "/status");
  auto readback2 = test.getScalar<int>("/Device2/MyModule/readBack");

  auto trigger = test.getScalar<int>("trigger");

  trigger.write();
  //device 1 is in Error state
  CHECK_TIMEOUT(message1.readLatest(), 10000);
  CHECK_TIMEOUT(status1.readLatest(), 10000);
  BOOST_CHECK_EQUAL(status1, 1);
  BOOST_CHECK(!readback1.readNonBlocking()); // error state at the beginning is not yet propagated

  // Device 2 might/will also come up in error state until the device is opened (which happends asynchronously in a
  // separate thread).
  /// So we have to read until we get a DataValidity::ok
  CHECK_TIMEOUT((readback2.readNonBlocking(), readback2.dataValidity() == ctk::DataValidity::ok), 10000);
  BOOST_CHECK_EQUAL(readback2, 110);

  // even with device 1 failing the second one must process the data, so send a new trigger
  // before fixing dev1
  dev2.write<int>("MyModule/readBack.DUMMY_WRITEABLE", 120);
  trigger.write();
  CHECK_TIMEOUT(readback2.readNonBlocking(), 10000); // device 2 still works
  BOOST_CHECK_EQUAL(readback2, 120);
  //Device is not in error state.
  CHECK_TIMEOUT(!message2.readLatest(), 10000);
  CHECK_TIMEOUT(!status2.readLatest(), 10000);

  //fix device 1
  dummyBackend1->throwExceptionOpen = false;
  //device 1 is fixed
  CHECK_TIMEOUT(message1.readLatest(), 10000);
  CHECK_TIMEOUT(status1.readLatest(), 10000);
  BOOST_CHECK_EQUAL(status1, 0);
  CHECK_TIMEOUT(readback1.readNonBlocking(), 10000);
  BOOST_CHECK_EQUAL(readback1, 100);
}

BOOST_AUTO_TEST_CASE(testConstants) {
  std::cout << "testConstants" << std::endl;
  // Constants are registered to the device to be written when opening/recovering
  // Attention: This test does not test that errors when writing to constants are displayed correctly. It only checks that witing when opeing and recovering works.
  TestApplication app;
  ctk::VariableNetworkNode::makeConstant<int32_t>(true, 18) >> app.dev1("/MyModule/actuator");
  app.cs("PleaseWriteToMe", typeid(int), 1) >> app.dev1("/Integers/signed32", typeid(int), 1);

  ctk::TestFacility test;
  test.runApplication();

  ChimeraTK::Device dev;
  dev.open(ExceptionDummyCDD1);

  // after opening a device the runApplication() might return, but the initialisation might not have happened in the other thread yet. So check with timeout.
  CHECK_TIMEOUT(dev.read<int32_t>("/MyModule/actuator") == 18, 10000);

  // So far this is also tested by testDeviceAccessors. Now cause errors.
  // Take back the value of the constant which was written to the device before making the device fail for further writes.
  dev.write<int32_t>("/MyModule/actuator", 0);
  auto dummyBackend = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  dummyBackend->throwExceptionWrite = true;

  auto pleaseWriteToMe = test.getScalar<int32_t>("/PleaseWriteToMe");
  pleaseWriteToMe = 42;
  pleaseWriteToMe.write();
  test.stepApplication(false);

  // Check that the error has been seen
  auto deviceStatus = test.getScalar<int32_t>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  deviceStatus.readLatest();
  BOOST_CHECK(deviceStatus == 1);

  // now cure the error
  dummyBackend->throwExceptionWrite = false;

  // Write something so we can call stepApplication to wake up the app.
  pleaseWriteToMe = 43;
  pleaseWriteToMe.write();
  test.stepApplication();

  CHECK_TIMEOUT(dev.read<int32_t>("/MyModule/actuator") == 18, 10000);
}

/// @todo FIXME: Write test that errors during constant writing are handled correctly, incl. correct error messages to the control system
BOOST_AUTO_TEST_CASE(testConstantWitingErrors) {}

BOOST_AUTO_TEST_CASE(testShutdown) {
  std::cout << "testShutdown" << std::endl;
  static const uint32_t DEFAULT = 55;

  auto dummyBackend1 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  auto dummyBackend2 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));
  auto dummyBackend3 = boost::dynamic_pointer_cast<ctk::ExceptionDummy>(
      ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD3));

  // Test that the application does shut down with a broken device and blocking accessors
  TestApplication2 app;
  ctk::TestFacility test(false); // test facility without testable mode

  ctk::Device dev2(ExceptionDummyCDD2);
  ctk::Device dev3(ExceptionDummyCDD3);

  // Non zero defaults set here to avoid race conditions documented in
  // https://github.com/ChimeraTK/ApplicationCore/issues/103
  test.setScalarDefault("/Device2/MyModule/actuator", static_cast<int32_t>(DEFAULT));
  test.setScalarDefault("/Device2/Integers/signed32", static_cast<int32_t>(DEFAULT));
  test.setScalarDefault("/Device2/Integers/unsigned32", static_cast<uint32_t>(DEFAULT));
  test.setScalarDefault("/Device2/Integers/signed16", static_cast<int16_t>(DEFAULT));
  test.setScalarDefault("/Device2/Integers/unsigned16", static_cast<uint16_t>(DEFAULT));
  test.setScalarDefault("/Device2/Integers/signed8", static_cast<int8_t>(DEFAULT));
  test.setScalarDefault("/Device2/Integers/unsigned8", static_cast<uint8_t>(DEFAULT));
  test.setScalarDefault("/Device2/FixedPoint/value", static_cast<double>(DEFAULT));
  test.setScalarDefault("/Device2/Deep/Hierarchies/Need/Tests/As/well", static_cast<int32_t>(DEFAULT));
  test.setScalarDefault("/Device2/Deep/Hierarchies/Need/Another/test", static_cast<int32_t>(DEFAULT));
  test.setScalarDefault("/Device3/MODULE/REG4", static_cast<int32_t>(DEFAULT));

  test.runApplication();
  app.inputModule.mainLoopStarted.wait();
  app.outputModule.mainLoopStarted.wait();
  app.realisticModule.mainLoopStarted.wait();

  // verify defaults have been written to the device
  CHECK_TIMEOUT(dev2.read<int32_t>("MyModule/actuator") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<int32_t>("Integers/signed32") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<uint32_t>("Integers/unsigned32") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<int16_t>("Integers/signed16") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<uint16_t>("Integers/unsigned16") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<int8_t>("Integers/signed8") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<uint8_t>("Integers/unsigned8") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<int32_t>("Deep/Hierarchies/Need/Tests/As/well") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev2.read<int32_t>("Deep/Hierarchies/Need/Another/test") == DEFAULT, 10000);
  CHECK_TIMEOUT(dev3.read<int32_t>("MODULE/REG4") == DEFAULT, 10000);

  // Wait for the devices to come up.
  CHECK_EQUAL_TIMEOUT(
      test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD1 / "status"), 0, 10000);
  CHECK_EQUAL_TIMEOUT(
      test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "status"), 0, 10000);
  CHECK_EQUAL_TIMEOUT(
      test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD3 / "status"), 0, 10000);

  // make all devices fail, and wait until they report the error state, one after another
  dummyBackend2->throwExceptionWrite = true;
  dummyBackend2->throwExceptionRead = true;

  // two blocking accessors on dev3: one for reading, one for writing
  auto trigger2 = test.getScalar<int32_t>("/trigger2");
  trigger2.write(); // triggers the read of readBack

  // wait for the error to be reported in the control system
  CHECK_EQUAL_TIMEOUT(
      test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "status"), 1, 10000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "message"),
      "DummyException: read throws by request", 10000);

  auto theInt = test.getScalar<int32_t>("/Device2/Integers/signed32");
  theInt.write();
  // the read is the first error we see. The second one is not reported any more for this device.
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "message"),
      "DummyException: read throws by request", 10000);

  // device 2 successfully broken!

  // block the output accessor of "outputModule
  dummyBackend1->throwExceptionWrite = true;
  dummyBackend1->throwExceptionRead = true;

  auto triggerActuator = test.getScalar<int32_t>("/triggerActuator");
  triggerActuator.write();

  // wait for the error to be reported in the control system
  CHECK_EQUAL_TIMEOUT(
      test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD1 / "status"), 1, 10000);
  // the write message does not have a \n, it is not going though a feeding fanout
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD1 / "message"),
      "DummyException: write throws by request", 10000);

  auto triggerReadback = test.getScalar<int32_t>("/triggerReadback");
  triggerReadback.write();

  // device 1 successfully broken!

  dummyBackend3->throwExceptionWrite =
      false; // do not set to true, otherwise there is a race condition whether the read or the write in RealisticModule::mainLoop() triggers the exception
  dummyBackend3->throwExceptionRead = true;

  auto triggerRealistic = test.getScalar<int32_t>("/triggerRealistic");
  triggerRealistic.write();

  CHECK_EQUAL_TIMEOUT(
      test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD3 / "status"), 1, 10000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD3 / "message"),
      "DummyException: read throws by request", 10000);

  auto reg4 = test.getScalar<int32_t>("/Device3/MODULE/REG4");
  reg4.write();

  // device 3 successfully broken!

  // I now blocked everything that comes to my mind.
  // And now the real test: does the test end or does it block when shuttig down?
}
