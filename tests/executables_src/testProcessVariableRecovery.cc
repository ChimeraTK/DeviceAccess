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

/* This module is the one which sees the exception because we will make it read from the 'broken' device.
 */
struct ReaderModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int32_t> trigger{this, "trigger2", "", "This is my trigger."};
  ctk::ScalarPollInput<int32_t> input{this, "FORM_DEV_SCALAR1", "", "Here I read a scalar"};
  ctk::ScalarOutput<int32_t> output{this, "twoTimesInput", "", "Twice what I read."};

  void mainLoop() override {
    while(true) {
      trigger.read();
      input.readLatest();
      output = 2 * input;
      output.write();
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
  ReaderModule readerModule{this, "TEST", "The reader module"};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testProcessVariableRecovery) {
  std::cout << "testProcessVariableRecovery" << std::endl;
  TestApplication app;
  app.findTag(".*").connectTo(app.cs); // creates /TEST/TO_DEV_SCALAR1 and /TEST/TO/DEV/ARRAY1
  // devices are not automatically connected (yet)
  app.dev.connectTo(
      app.cs); // In TEST it connects to TO_DEV_SCALAR1 and TO_DEV_ARRAY1, and creates TO_DEV_SCALAR2, FROM_DEV1, FROM_DEV2, TO_DEV_AREA2, FROM_DEV_AREA1 and FROM_DEV_AREA2

  ctk::TestFacility test(false);
  // initial value for the direct CS->DEV register
  test.writeScalar("/TEST/TO_DEV_SCALAR2", 42);
  std::vector<int32_t> array = {99, 99, 99, 99};
  test.writeArray("/TEST/TO_DEV_ARRAY2", array);

  app.run();
  app.dumpConnections();

  ctk::Device dummy;
  dummy.open(deviceCDD);
  //Check that the initial values are there.
  //auto reg2 = dummy.getScalarRegisterAccessor<int32_t>("/TEST/TO_DEV_SCALAR2");
  //CHECK_EQUAL_TIMEOUT([=]()mutable{reg2.readLatest(); return int32_t(reg2);},0,3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR2"), 42, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 0)[0], 99, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 1)[0], 99, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 2)[0], 99, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 3)[0], 99, 3000);

  //Update device register via application module.
  auto trigger = test.getScalar<int32_t>("/TEST/trigger");
  trigger = 100;
  trigger.write();
  //Check if the values are updated.
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR1"), 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 0)[0], 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 1)[0], 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 2)[0], 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 3)[0], 100, 3000);

  auto dummyBackend =
      boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend(deviceCDD));

  //Set the device to throw.
  dummyBackend->throwExceptionOpen = true;

  //Set dummy registers to 0.
  dummy.write<int32_t>("/TEST/TO_DEV_SCALAR1", 0);
  dummy.write<int32_t>("/TEST/TO_DEV_SCALAR2", 0);
  array = {0, 0, 0, 0};
  dummy.write("/TEST/TO_DEV_ARRAY1", array);
  dummy.write("/TEST/TO_DEV_ARRAY2", array);

  /*
   * Set the device to throw on write so that when application module
   * tries to update the device register device goes into error state.
  */
  dummyBackend->throwExceptionWrite = true;
  trigger = 100;
  trigger.write();

  //Verify that the device is in error state.
  CHECK_EQUAL_TIMEOUT(test.readScalar<int32_t>(ctk::RegisterPath("/Devices") / deviceCDD / "status"), 1, 3000);

  //Set device back to normal.
  dummyBackend->throwExceptionOpen = false;
  dummyBackend->throwExceptionWrite = false;
  //Verify if the device is ready.
  CHECK_EQUAL_TIMEOUT(dummyBackend->isFunctional(), 1, 3000);

  //Device should have the correct values now.
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR2"), 42, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 0)[0], 99, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 1)[0], 99, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 2)[0], 99, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY2", 1, 3)[0], 99, 3000);

  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_SCALAR1"), 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 0)[0], 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 1)[0], 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 2)[0], 100, 3000);
  CHECK_EQUAL_TIMEOUT(dummy.read<int32_t>("/TEST/TO_DEV_ARRAY1", 1, 3)[0], 100, 3000);
}
