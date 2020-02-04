#define BOOST_TEST_MODULE testProcessVariableRecovery

#include <boost/test/included/unit_test.hpp>
#include "Application.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "TestFacility.h"
#include "ExceptionDevice.h"
#include <ChimeraTK/Device.h>
#include <stdlib.h>
#include "check_timeout.h"
#include "ApplicationModule.h"
#include "ArrayAccessor.h"
#include "ConfigReader.h"

#include <regex>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

static constexpr char deviceCDD[] = "(ExceptionDummy?map=test5.map)";

/* The test module is writing to the device. It is the "module under test".
 * This is the one whose variables are to be recovered. It is not the place the the
 * application first sees the exception.
 */
struct TestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int32_t> trigger{this, "trigger", "", "This is my trigger."};
  ctk::ScalarOutput<int32_t> scalarOutput{this, "TO_DEV_SCALAR1", "", "Here I write a scalar"};
  ctk::ArrayOutput<int32_t> arrayOutput{this, "TO_DEV_ARRAY1", "", 4, "Here I write an array"};

  void mainLoop() override {
    while(true) {
      trigger.read();
      scalarOutput = int32_t(trigger);
      scalarOutput.write();
      for(uint i = 0; i < 4; i++) {
        arrayOutput[i] = int32_t(trigger);
      }
      arrayOutput.write();
    }
  }
};

/* dummy application */
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests
  ctk::ControlSystemModule cs;
  ctk::DeviceModule dev{this, deviceCDD};
  TestModule module{this, "TEST", "The test module"};
};

/* Test application for the specific case of wrinting to a read-only
 * accessor. Provides an input to an ApplicationModule from a read-only
 * accessor of the device. For the test, the accessor must not be routed
 * through the control system, the illegal write would be catched by the
 * ControlSystemAdapter, not by the ExceptionHandlingDecorator under test here.
 */
struct ReadOnlyTestApplication : public ctk::Application {
  ReadOnlyTestApplication() : Application("ReadOnlytestApp") {}
  ~ReadOnlyTestApplication() { shutdown(); }

  void defineConnections() {
    dev["TEST"]("FROM_DEV_SCALAR2") >> module("FROM_DEV_SCALAR2");
    findTag("CS").connectTo(cs);
  }

  ctk::ControlSystemModule cs;
  ctk::DeviceModule dev{this, deviceCDD};

  struct TestModule : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInput<int> start{
        this, "startTest", "", "This has to be written once, before writing to the device", {"CS"}};
    ctk::ScalarPollInput<int32_t> scalarROInput{this, "FROM_DEV_SCALAR2", "", "Here I read from a scalar RO-register"};

    void mainLoop() override {
      // Just to have a blocking read, gives the test
      // time to dumpConnections and explicitly trigger
      // before terminating.
      start.read();

      scalarROInput = 42;
      try {
        scalarROInput.write();
        BOOST_CHECK_MESSAGE(
            false, "ReadOnlyTestApplication: Calling write() on input to read-only device register did not throw.");
      }
      catch(ChimeraTK::logic_error& e) {
        const std::string exMsg{e.what()};
        std::regex exceptionHandlingRegex{"^ChimeraTK::ExceptionhandlingDecorator:*"};
        std::smatch exMatch;

        BOOST_CHECK(std::regex_search(exMsg, exMatch, exceptionHandlingRegex));
      }
    }

  } module{this, "READ_ONLY_TEST", "The test module"};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteToReadOnly) {
  std::cout << "testWriteToReadOnly" << std::endl;

  ReadOnlyTestApplication app;

  ctk::TestFacility test;

  test.runApplication();

  // Should trigger the blocking read in ReadOnlyTestApplication's
  // ApplicationModule. It then writes to a read-only register of the device,
  // which should throw. Check is done in the module's mainLoop. We can not check
  // here, as the exception gets thrown in the thread of the module.
  test.writeScalar("/READ_ONLY_TEST/startTest", 1);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testProcessVariableRecovery) {
  std::cout << "testProcessVariableRecovery" << std::endl;
  TestApplication app;

  app.findTag(".*").connectTo(app.cs); // creates /TEST/TO_DEV_SCALAR1 and /TEST/TO/DEV/ARRAY1
  // devices are not automatically connected (yet)
  app.dev.connectTo(app.cs,
      app.cs("deviceTrigger", typeid(int),
          1)); // In TEST it connects to TO_DEV_SCALAR1 and TO_DEV_ARRAY1, and creates TO_DEV_SCALAR2, FROM_DEV1, FROM_DEV2, TO_DEV_AREA2, FROM_DEV_AREA1 and FROM_DEV_AREA2

  // make a constant and connect to the device
  auto constante = ctk::VariableNetworkNode::makeConstant(1, 44252, 1);
  constante >> app.dev["CONSTANT"]("VAR32");

  ctk::TestFacility test(false);
  // initial value for the direct CS->DEV register
  test.writeScalar("/TEST/TO_DEV_SCALAR2", 42);
  std::vector<int32_t> array = {99, 99, 99, 99};
  test.writeArray("/TEST/TO_DEV_ARRAY2", array);

  // write initial values since we do not use the test facility for the app management
  test.writeScalar("/TEST/trigger", 0);

  app.run();

  ctk::Device dummy;
  dummy.open(deviceCDD);
  //Check that the initial values are there.
  //auto reg2 = dummy.getScalarRegisterAccessor<int32_t>("/TEST/TO_DEV_SCALAR2");
  //CHECK_EQUAL_TIMEOUT([=]()mutable{reg2.readLatest(); return int32_t(reg2);},0,10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/CONSTANT/VAR32"), 44252, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR2"), 42, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 0)[0], 99, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 1)[0], 99, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 2)[0], 99, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 3)[0], 99, 10000);

  //Update device register via application module.
  auto trigger = test.getScalar<int32_t>("/TEST/trigger");
  trigger = 100;
  trigger.write();
  //Check if the values are updated.
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR1"), 100, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 0)[0], 100, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 1)[0], 100, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 2)[0], 100, 10000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 3)[0], 100, 10000);

  auto dummyBackend =
      boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend(deviceCDD));

  //Set the device to throw.
  dummyBackend->throwExceptionOpen = true;

  //Set dummy registers to 0.
  dummy.write<int32_t>("/CONSTANT/VAR32", 0);
  dummy.write<int32_t>("/TEST/TO_DEV_SCALAR1", 0);
  dummy.write<int32_t>("/TEST/TO_DEV_SCALAR2", 0);
  array = {0, 0, 0, 0};
  dummy.write("/TEST/TO_DEV_ARRAY1", array);
  dummy.write("/TEST/TO_DEV_ARRAY2", array);

  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/CONSTANT/VAR32"), 0, 10000);
  dummyBackend->throwExceptionWrite = true;
  dummyBackend->throwExceptionRead = true;

  // Now we trigger the reading module. This should put the device into an error state
  auto trigger2 = test.getScalar<int32_t>("/deviceTrigger");
  trigger2.write();

  //Verify that the device is in error state.
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / deviceCDD / "status"), 1, 10000);

  //Set device back to normal.
  dummyBackend->throwExceptionWrite = false;
  dummyBackend->throwExceptionRead = false;
  dummyBackend->throwExceptionOpen = false;
  //Verify if the device is ready.
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / deviceCDD / "status"), 0, 10000);

  //Device should have the correct values now. Notice that we did not trigger the writer module!
  BOOST_CHECK_EQUAL(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR2"), 42);
  BOOST_CHECK((dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 0) == std::vector<int32_t>{99, 99, 99, 99}));

  BOOST_CHECK_EQUAL(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR1"), 100);
  BOOST_CHECK((dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 0) == std::vector<int32_t>{100, 100, 100, 100}));

  // check if the constant is written back after recovery
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/CONSTANT/VAR32"), 44252, 10000);
}
