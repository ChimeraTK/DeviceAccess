#include <future>

#define BOOST_TEST_MODULE testExceptionHandling

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/DummyRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ExceptionDevice.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "check_timeout.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

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
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int32_t> trigger{this, "trigger", "", "I wait for this to start."};
  ctk::ScalarOutput<int32_t> actuator{this, "actuator", "", "This is where I write to."};

  void mainLoop() override {
    trigger.read();
    actuator = int32_t(trigger);
    actuator.write();
  }
};

struct InputModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int32_t> trigger{this, "trigger", "", "I wait for this to start."};
  ctk::ScalarPollInput<int32_t> readback{this, "readback", "", "Just going to read something."};

  void mainLoop() override {
    trigger.read();
    readback.read();
    // not very useful because I am doing anything with the read values, but still a useful test
  }
};

struct RealisticModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int32_t> reg1{this, "REG1", "", "misused as input"};
  ctk::ScalarPollInput<int32_t> reg2{this, "REG2", "", "also no input..."};
  ctk::ScalarOutput<int32_t> reg3{this, "REG3", "", "my output"};

  void mainLoop() override {
    reg1.read();
    reg2.readLatest();

    reg3 = reg1 * reg2;
    reg3.write();
  }
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
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> readbackDummy1(dummyBackend1.get(), "MyModule", "readBack");
  ChimeraTK::DummyRegisterAccessor<int> readbackDummy2(dummyBackend2.get(), "MyModule", "readBack");

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

  readbackDummy1 = 42;
  readbackDummy2 = 52;

  // initially there should be no error set
  trigger.write();
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());
  CHECK_TIMEOUT(readback1.readLatest(), 1000);
  CHECK_TIMEOUT(readback2.readLatest(), 1000);
  BOOST_CHECK(static_cast<std::string>(message1) == "");
  BOOST_CHECK(status1 == 0);
  BOOST_CHECK_EQUAL(readback1, 42);
  BOOST_CHECK_EQUAL(readback2, 52);

  // repeat test a couple of times to make sure it works not only once
  for(size_t i = 0; i < 3; ++i) {
    // enable exception throwing in test device 1
    readbackDummy1 = 10 + i;
    readbackDummy2 = 20 + i;
    dummyBackend1->throwExceptionRead = true;
    trigger.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    BOOST_CHECK(readback1.readNonBlocking());                                 // we have been signalized new data
    BOOST_CHECK(readback1.dataValidity() == ChimeraTK::DataValidity::faulty); // But the fault flag should be set
    // the second device must still be functional
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(readback2.readNonBlocking(), 1000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 20 + i);

    // even with device 1 failing the second one must process the data, so send a new trigger
    // before fixing dev1
    readbackDummy2 = 120 + i;
    trigger.write();
    BOOST_CHECK(!readback1.readNonBlocking());                                // we should not have gotten any new data
    BOOST_CHECK(readback1.dataValidity() == ChimeraTK::DataValidity::faulty); // But the fault flag should still be set
    CHECK_TIMEOUT(readback2.readNonBlocking(), 1000);                         // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 120 + i);

    // Now "cure" the device problem
    readbackDummy1 = 30 + i;
    readbackDummy2 = 40 + i;
    dummyBackend1->throwExceptionRead = false;
    trigger.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(status1, 0);
    BOOST_CHECK_EQUAL(readback1, 30 + i);
    BOOST_CHECK(readback1.dataValidity() == ChimeraTK::DataValidity::ok); // The fault flag should have been cleared
    // there are two more copies in the queue, since the two triggers received during the error state is still
    // processed after recovery
    CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
    BOOST_CHECK_EQUAL(readback1, 30 + i);
    CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
    BOOST_CHECK_EQUAL(readback1, 30 + i);
    BOOST_CHECK(!readback1.readNonBlocking()); // now the queue should be empty
    // device2
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(readback2.readNonBlocking(), 1000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 40 + i);
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandlingWrite) {
  std::cout << "testExceptionHandlingWrite" << std::endl;
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> actuatorDummy1(dummyBackend1.get(), "MyModule", "actuator");
  ChimeraTK::DummyRegisterAccessor<int> actuatorDummy2(dummyBackend2.get(), "MyModule", "actuator");

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

  // initially there should be no error set
  actuator1 = 29;
  actuator1.write();
  actuator2 = 39;
  actuator2.write();
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());
  CHECK_TIMEOUT(actuatorDummy1 == 29, 1000);
  CHECK_TIMEOUT(actuatorDummy2 == 39, 1000);
  BOOST_CHECK(static_cast<std::string>(message1) == "");
  BOOST_CHECK(status1 == 0);

  // repeat test a couple of times to make sure it works not only once
  for(size_t i = 0; i < 3; ++i) {
    // enable exception throwing in test device 1
    dummyBackend1->throwExceptionWrite = true;
    actuator1 = 30 + i;
    actuator1.write();
    actuator2 = 40 + i;
    actuator2.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    usleep(10000);                                  // 10ms wait time so potential wrong values could have propagated
    BOOST_CHECK(actuatorDummy1 == int(30 + i - 1)); // write not done for broken device
    // the second device must still be functional
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(actuatorDummy2 == int(40 + i), 1000); // device 2 still works

    // even with device 1 failing the second one must process the data, so send a new data before fixing dev1
    actuator2 = 120 + i;
    actuator2.write();
    CHECK_TIMEOUT(actuatorDummy2 == int(120 + i), 1000); // device 2 still works

    // Now "cure" the device problem
    dummyBackend1->throwExceptionWrite = false;
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    CHECK_TIMEOUT(actuatorDummy1 == int(30 + i), 1000); // write is now complete
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(status1, 0);
  }
}
/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandlingOpen) {
  std::cout << "testExceptionHandlingOpen" << std::endl;
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> readbackDummy1(dummyBackend1.get(), "MyModule", "readBack");
  ChimeraTK::DummyRegisterAccessor<int> readbackDummy2(dummyBackend2.get(), "MyModule", "readBack");

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

  readbackDummy1 = 100;
  readbackDummy2 = 110;
  trigger.write();
  //device 1 is in Error state
  CHECK_TIMEOUT(message1.readLatest(), 1000);
  CHECK_TIMEOUT(status1.readLatest(), 1000);
  BOOST_CHECK_EQUAL(status1, 1);
  BOOST_CHECK(!readback1.readNonBlocking());
  CHECK_TIMEOUT(readback2.readNonBlocking(), 1000);
  BOOST_CHECK_EQUAL(readback2, 110);

  // even with device 1 failing the second one must process the data, so send a new trigger
  // before fixing dev1
  readbackDummy2 = 120;
  trigger.write();
  CHECK_TIMEOUT(readback2.readNonBlocking(), 1000); // device 2 still works
  BOOST_CHECK_EQUAL(readback2, 120);
  //Device is not in error state.
  CHECK_TIMEOUT(!message2.readLatest(), 1000);
  CHECK_TIMEOUT(!status2.readLatest(), 1000);

  //fix device 1
  dummyBackend1->throwExceptionOpen = false;
  //device 1 is fixed
  CHECK_TIMEOUT(message1.readLatest(), 1000);
  CHECK_TIMEOUT(status1.readLatest(), 1000);
  BOOST_CHECK_EQUAL(status1, 0);
  CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
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
  CHECK_TIMEOUT(dev.read<int32_t>("/MyModule/actuator") == 18, 3000);

  // So far this is also tested by testDeviceAccessors. Now cause errors.
  // Take back the value of the constant which was written to the device before making the device fail for further writes.
  dev.write<int32_t>("/MyModule/actuator", 0);
  auto dummyBackend =
      boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  dummyBackend->throwExceptionWrite = true;

  auto pleaseWriteToMe = test.getScalar<int32_t>("/PleaseWriteToMe");
  pleaseWriteToMe = 42;
  pleaseWriteToMe.write();
  test.stepApplication();

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

  CHECK_TIMEOUT(dev.read<int32_t>("/MyModule/actuator") == 18, 3000);
}

/// @todo FIXME: Write test that errors during constant writing are handled correctly, incl. correct error messages to the control system
BOOST_AUTO_TEST_CASE(testConstantWitingErrors) {}

BOOST_AUTO_TEST_CASE(testShutdown) {
  std::cout << "testShutdown" << std::endl;
  //Test that the application does shut down with a broken device and blocking accessors
  TestApplication2 app;

  ctk::TestFacility test(false); // test facility without testable mode

  app.initialise();
  app.run();
  //app.dumpConnections();

  //Wait for the devices to come up.
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD1 / "status"), 0, 3000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "status"), 0, 3000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD3 / "status"), 0, 3000);

  // make all devices fail, and wait until they report the error state, one after another
  auto dummyBackend2 =
      boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));
  dummyBackend2->throwExceptionWrite = true;
  dummyBackend2->throwExceptionRead = true;

  // two blocking accessors on dev3: one for reading, one for writing
  auto trigger2 = test.getScalar<int32_t>("/trigger2");
  trigger2.write(); // triggers the read of readBack

  // wait for the error to be reported in the control system
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "status"), 1, 3000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "message"),
      "DummyException: read throws by request", 3000);

  auto theInt = test.getScalar<int32_t>("/Device2/Integers/signed32");
  theInt.write();
  // the read is the first error we see. The second one is not reported any more for this device.
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD2 / "message"),
      "DummyException: read throws by request", 3000);

  // device 2 successfully broken!

  // block the output accessor of "outputModule
  auto dummyBackend1 =
      boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  dummyBackend1->throwExceptionWrite = true;
  dummyBackend1->throwExceptionRead = true;

  auto triggerActuator = test.getScalar<int32_t>("/triggerActuator");
  triggerActuator.write();

  // wait for the error to be reported in the control system
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD1 / "status"), 1, 3000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD1 / "message"),
      "DummyException: write throws by request", 3000);

  auto triggerReadback = test.getScalar<int32_t>("/triggerReadback");
  triggerReadback.write();

  // device 1 successfully broken!

  auto dummyBackend3 =
      boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend(ExceptionDummyCDD3));
  dummyBackend3->throwExceptionWrite = true;
  dummyBackend3->throwExceptionRead = true;

  auto triggerRealistic = test.getScalar<int32_t>("/triggerRealistic");
  triggerRealistic.write();

  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD3 / "status"), 1, 3000);
  CHECK_EQUAL_TIMEOUT(test.readScalar<std::string>(ctk::RegisterPath("/Devices") / ExceptionDummyCDD3 / "message"),
      "DummyException: read throws by request", 3000);

  auto reg4 = test.getScalar<int32_t>("/Device3/MODULE/REG4");
  reg4.write();

  // device 3 successfully broken!

  // I now blocked everything that comes to my mind.
  // And now the real test: does the test end or does it block when shuttig down?
}
