#include <future>

#define BOOST_TEST_MODULE testPropagateDataFaultFlag

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/DummyRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "TestFacility.h"
#include "ModuleGroup.h"
#include "check_timeout.h"

#include <ChimeraTK/ExceptionDevice.h>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/* dummy application */

struct TestModule1 : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPushInput<int> i1{this, "i1", "", ""};
  ctk::ArrayPushInput<int> i2{this, "i2", "", 2, ""};
  ctk::ScalarPushInputWB<int> i3{this, "i3", "", ""};
  ctk::ScalarOutput<int> o1{this, "o1", "", ""};
  ctk::ArrayOutput<int> o2{this, "o2", "", 2, ""};
  void mainLoop() override {
    auto group = readAnyGroup();
    while(true) {
      group.readAny();
      if(i3 > 10) {
        i3 = 10;
        i3.write();
      }
      o1 = int(i1);
      o2[0] = i2[0];
      o2[1] = i2[1];
      o1.write();
      o2.write();
    }
  }
};

struct TestApplication1 : ctk::Application {
  TestApplication1() : Application("testSuite") {}
  ~TestApplication1() { shutdown(); }

  void defineConnections() { t1.connectTo(cs); }

  TestModule1 t1{this, "t1", ""};
  ctk::ControlSystemModule cs;
};

struct TestApplication2 : ctk::Application {
  TestApplication2() : Application("testSuite") {}
  ~TestApplication2() { shutdown(); }

  void defineConnections() {
    t1.connectTo(cs["A"]);
    t1.connectTo(cs["B"]);
  }

  TestModule1 t1{this, "t1", ""};
  ctk::ControlSystemModule cs;
};
/*********************************************************************************************************************/

// first test without FanOuts of any kind
BOOST_AUTO_TEST_CASE(testDirectConnections) {
  std::cout << "testDirectConnections" << std::endl;
  TestApplication1 app;
  ctk::TestFacility test;

  auto i1 = test.getScalar<int>("i1");
  auto i2 = test.getArray<int>("i2");
  auto i3 = test.getScalar<int>("i3");
  auto o1 = test.getScalar<int>("o1");
  auto o2 = test.getArray<int>("o2");

  test.runApplication();

  // test if fault flag propagates to all outputs
  i1 = 1;
  i1.setDataValidity(ctk::DataValidity::faulty);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 1);
  BOOST_CHECK_EQUAL(o2[0], 0);
  BOOST_CHECK_EQUAL(o2[1], 0);

  // write another value but keep fault flag
  i1 = 42;
  BOOST_CHECK(i1.dataValidity() == ctk::DataValidity::faulty);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 42);
  BOOST_CHECK_EQUAL(o2[0], 0);
  BOOST_CHECK_EQUAL(o2[1], 0);

  // a write on the ok variable should not clear the flag
  i2[0] = 10;
  i2[1] = 11;
  BOOST_CHECK(i2.dataValidity() == ctk::DataValidity::ok);
  i2.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 42);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);

  // the return channel should also receive the flag
  BOOST_CHECK(i3.readNonBlocking() == false);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::ok);
  i3 = 20;
  i3.write();
  test.stepApplication();
  o1.read();
  o2.read();
  i3.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 42);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // clear the flag on i1, i3 will keep it for now (we have received it there and not yet sent it out!)
  i1 = 3;
  i1.setDataValidity(ctk::DataValidity::ok);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(i3.readNonBlocking() == false);
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(o1), 3);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // send two data fault flags. both need to be cleared before the outputs go back to ok
  i1 = 120;
  i1.setDataValidity(ctk::DataValidity::faulty);
  i1.write();
  i3 = 121;
  i3.write();
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  test.stepApplication();
  o1.readLatest();
  o2.readLatest();
  i3.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 120);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // clear first flag
  i1 = 122;
  i1.setDataValidity(ctk::DataValidity::ok);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(i3.readNonBlocking() == false);
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 122);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // clear second flag
  i3 = 123;
  i3.setDataValidity(ctk::DataValidity::ok);
  i3.write();
  test.stepApplication();
  o1.read();
  o2.read();
  i3.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(o1), 122);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(i3), 10);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWithFanOut) {
  std::cout << "testWithFanOut" << std::endl;
  TestApplication2 app;
  ctk::TestFacility test;

  auto Ai1 = test.getScalar<int>("A/i1");
  auto Ai2 = test.getArray<int>("A/i2");
  auto Ai3 = test.getScalar<int>("A/i3");
  auto Ao1 = test.getScalar<int>("A/o1");
  auto Ao2 = test.getArray<int>("A/o2");
  auto Bi1 = test.getScalar<int>("B/i1");
  auto Bi2 = test.getArray<int>("B/i2");
  auto Bi3 = test.getScalar<int>("B/i3");
  auto Bo1 = test.getScalar<int>("B/o1");
  auto Bo2 = test.getArray<int>("B/o2");

  test.runApplication();

  //app.dumpConnections();

  // test if fault flag propagates to all outputs
  Ai1 = 1;
  Ai1.setDataValidity(ctk::DataValidity::faulty);
  Ai1.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi1.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Ao1), 1);
  BOOST_CHECK_EQUAL(Ao2[0], 0);
  BOOST_CHECK_EQUAL(Ao2[1], 0);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bo1), 1);
  BOOST_CHECK_EQUAL(Bo2[0], 0);
  BOOST_CHECK_EQUAL(Bo2[1], 0);
  BOOST_CHECK(Bi1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bi1), 1);

  // send fault flag on a second variable
  Ai2[0] = 2;
  Ai2[1] = 3;
  Ai2.setDataValidity(ctk::DataValidity::faulty);
  Ai2.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi2.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Ao1), 1);
  BOOST_CHECK_EQUAL(Ao2[0], 2);
  BOOST_CHECK_EQUAL(Ao2[1], 3);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bo1), 1);
  BOOST_CHECK_EQUAL(Bo2[0], 2);
  BOOST_CHECK_EQUAL(Bo2[1], 3);
  BOOST_CHECK(Bi2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(Bi2[0], 2);
  BOOST_CHECK_EQUAL(Bi2[1], 3);

  // clear fault flag on a second variable
  Ai2[0] = 4;
  Ai2[1] = 5;
  Ai2.setDataValidity(ctk::DataValidity::ok);
  Ai2.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi2.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Ao1), 1);
  BOOST_CHECK_EQUAL(Ao2[0], 4);
  BOOST_CHECK_EQUAL(Ao2[1], 5);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bo1), 1);
  BOOST_CHECK_EQUAL(Bo2[0], 4);
  BOOST_CHECK_EQUAL(Bo2[1], 5);
  BOOST_CHECK(Bi2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(Bi2[0], 4);
  BOOST_CHECK_EQUAL(Bi2[1], 5);

  // clear fault flag on a first variable
  Ai1 = 6;
  Ai1.setDataValidity(ctk::DataValidity::ok);
  Ai1.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi1.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(Ao1), 6);
  BOOST_CHECK_EQUAL(Ao2[0], 4);
  BOOST_CHECK_EQUAL(Ao2[1], 5);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(Bo1), 6);
  BOOST_CHECK_EQUAL(Bo2[0], 4);
  BOOST_CHECK_EQUAL(Bo2[1], 5);
  BOOST_CHECK(Bi1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(Bi1), 6);
}

/*********************************************************************************************************************/
/*
 * Tests below verify data fault flag propagation on:
 * - Threaded FanOut
 * - Consuming FanOut
 * - Triggers
 */

struct Module1 : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPushInput<int> fromThreadedFanout{this, "o1", "", "", {"DEVICE1", "CS"}};

  // As a workaround the device side connection is done manually for
  // acheiving this consumingFanout; see:
  // TestApplication3::defineConnections
  ctk::ScalarPollInput<int> fromConsumingFanout{this, "i1", "", "", {"CS"}};

  ctk::ScalarPollInput<int> fromDevice{this, "i2", "", "", {"DEVICE2"}};
  ctk::ScalarOutput<int> result{this, "Module1_result", "", "", {"CS"}};

  void mainLoop() override {
    while(true) {
      readAll();
      result = fromConsumingFanout + fromThreadedFanout + fromDevice;
      writeAll();
    }
  }
};

struct Module2 : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  struct : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<int> result{this, "Module1_result", "", "", {"CS"}};
  } m1VarsFromCS{this, "m1", "", ctk::HierarchyModifier::oneLevelUp}; // "m1" being in there
                                                                      // not good for a general case
  ctk::ScalarOutput<int> result{this, "Module2_result", "", "", {"CS"}};

  void mainLoop() override {
    while(true) {
      readAll();
      result = static_cast<int>(m1VarsFromCS.result);
      writeAll();
    }
  }
};

// struct TestApplication3 : ctk::ApplicationModule {
struct TestApplication3 : ctk::Application {
  /*
 *   CS +-----> threaded fanout +------------------+
 *                  +                              v
 *                  +---------+                   +Device1+
 *                            |                   |       |
 *              Feeding       v                   |       |
 *   CS   <----- fanout --+ Module1 <-----+       v       |
 *                 |          ^           +Consuming      |
 *                 |          +--------+    fanout        |
 *                 +------+            +      +           |
 *                        v         Device2   |           |
 *   CS   <-----------+ Module2               |           |
 *                                            |           |
 *   CS   <-----------------------------------+           |
 *                                                        |
 *                                                        |
 *   CS   <-----------+ Trigger fanout <------------------+
 *                           ^
 *                           |
 *                           +
 *                           CS
 */

  constexpr static char const* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=testDataValidity1.map)";
  constexpr static char const* ExceptionDummyCDD2 = "(ExceptionDummy:1?map=testDataValidity2.map)";
  TestApplication3() : Application("testDataFlagPropagation") {}
  ~TestApplication3() { shutdown(); }

  Module1 m1{this, "m1", ""};
  Module2 m2{this, "m2", ""};

  ctk::ControlSystemModule cs;
  ctk::DeviceModule device1DummyBackend{this, ExceptionDummyCDD1};
  ctk::DeviceModule device2DummyBackend{this, ExceptionDummyCDD2};

  void defineConnections() override {
    device1DummyBackend["m1"]("i1") >> m1("i1");
    findTag("CS").connectTo(cs);
    findTag("DEVICE1").connectTo(device1DummyBackend);
    findTag("DEVICE2").connectTo(device2DummyBackend);
    device1DummyBackend["m1"]("i3")[cs("trigger", typeid(int), 1)] >> cs("i3", typeid(int), 1);
  }
};

struct Fixture_testFacility {
  Fixture_testFacility()
  : device1DummyBackend(boost::dynamic_pointer_cast<ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(TestApplication3::ExceptionDummyCDD1))),
    device2DummyBackend(boost::dynamic_pointer_cast<ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(TestApplication3::ExceptionDummyCDD2))) {
    device1DummyBackend->open();
    device2DummyBackend->open();
    test.runApplication();
  }

  ~Fixture_testFacility() {
    device1DummyBackend->throwExceptionRead = false;
    device2DummyBackend->throwExceptionWrite = false;
  }

  boost::shared_ptr<ExceptionDummy> device1DummyBackend;
  boost::shared_ptr<ExceptionDummy> device2DummyBackend;
  TestApplication3 app;
  ctk::TestFacility test;
};

BOOST_FIXTURE_TEST_SUITE(data_validity_propagation, Fixture_testFacility)

BOOST_AUTO_TEST_CASE(testThreadedFanout) {
  std::cout << "testThreadedFanout" << std::endl;
  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto m1_result = test.getScalar<int>("m1/Module1_result");
  auto m2_result = test.getScalar<int>("m2/Module2_result");

  threadedFanoutInput = 20;
  threadedFanoutInput.write();
  // write to register: m1.i1 linked with the consumingFanout.
  auto consumingFanoutSource = device1DummyBackend->getRawAccessor("m1", "i1");
  consumingFanoutSource = 10;

  auto pollRegister = device2DummyBackend->getRawAccessor("m1", "i2");
  pollRegister = 5;

  test.stepApplication();

  m1_result.read();
  m2_result.read();
  BOOST_CHECK_EQUAL(m1_result, 35);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::ok);

  BOOST_CHECK_EQUAL(m2_result, 35);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::ok);

  threadedFanoutInput = 10;
  threadedFanoutInput.setDataValidity(ctk::DataValidity::faulty);
  threadedFanoutInput.write();
  test.stepApplication();

  m1_result.read();
  m2_result.read();
  BOOST_CHECK_EQUAL(m1_result, 25);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(m2_result, 25);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::faulty);

  threadedFanoutInput = 40;
  threadedFanoutInput.setDataValidity(ctk::DataValidity::ok);
  threadedFanoutInput.write();
  test.stepApplication();

  m1_result.read();
  m2_result.read();
  BOOST_CHECK_EQUAL(m1_result, 55);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(m2_result, 55);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_CASE(testInvalidTrigger) {
  std::cout << "testInvalidTrigger" << std::endl;
  return; // FIXME Test does not pass because feature is not implemented yet.
          // See issue #109
  auto deviceRegister = device1DummyBackend->getRawAccessor("m1", "i3");
  deviceRegister = 20;

  auto trigger = test.getScalar<int>("trigger");
  auto result = test.getScalar<int>("i3"); //Cs hook into reg: m1.i3

  //----------------------------------------------------------------//
  // trigger works as expected
  trigger = 1;
  trigger.write();

  test.stepApplication();

  result.read();
  BOOST_CHECK_EQUAL(result, 20);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  //----------------------------------------------------------------//
  // faulty trigger
  deviceRegister = 30;
  trigger = 1;
  trigger.setDataValidity(ctk::DataValidity::faulty);
  trigger.write();

  test.stepApplication();

  result.read();
  BOOST_CHECK_EQUAL(result, 30);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::faulty);

  //----------------------------------------------------------------//
  // recovery
  deviceRegister = 50;

  trigger = 1;
  trigger.setDataValidity(ctk::DataValidity::ok);
  trigger.write();

  test.stepApplication();

  result.read();
  BOOST_CHECK_EQUAL(result, 50);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_SUITE_END()

struct Fixture_noTestableMode {
  Fixture_noTestableMode()
  : device1DummyBackend(boost::dynamic_pointer_cast<ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(TestApplication3::ExceptionDummyCDD1))),
    device2DummyBackend(boost::dynamic_pointer_cast<ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(TestApplication3::ExceptionDummyCDD2))) {
    device1DummyBackend->open();
    device2DummyBackend->open();

    // the block below is a work around for a race condition; make sure
    // all values are propagated to the device registers before starting
    // the test.
    static const int DEFAULT = 1;
    test.setScalarDefault("m1/o1", DEFAULT);

    test.runApplication();

    // Making sure the default is written to the device before proceeding.
    CHECK_EQUAL_TIMEOUT(int(device1DummyBackend->getRawAccessor("m1", "o1")), DEFAULT, 10000);
  }

  ~Fixture_noTestableMode() {
    device1DummyBackend->throwExceptionRead = false;
    device2DummyBackend->throwExceptionWrite = false;
  }

  boost::shared_ptr<ExceptionDummy> device1DummyBackend;
  boost::shared_ptr<ExceptionDummy> device2DummyBackend;
  TestApplication3 app;
  ctk::TestFacility test{false};
};

BOOST_FIXTURE_TEST_SUITE(data_validity_propagation_noTestFacility, Fixture_noTestableMode)

BOOST_AUTO_TEST_CASE(testDeviceReadFailure) {
  std::cout << "testDeviceReadFailure" << std::endl;
  auto consumingFanoutSource = device1DummyBackend->getRawAccessor("m1", "i1");
  auto pollRegister = device2DummyBackend->getRawAccessor("m1", "i2");

  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto result = test.getScalar<int>("m1/Module1_result");

  threadedFanoutInput = 10000;
  consumingFanoutSource = 1000;
  pollRegister = 1;

  // -------------------------------------------------------------//
  // without errors
  threadedFanoutInput.write();

  CHECK_TIMEOUT((result.readLatest(), result == 11001), 10000);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  // -------------------------------------------------------------//
  // device module exception
  pollRegister = 0;

  device2DummyBackend->throwExceptionRead = true;

  threadedFanoutInput.write();
  // when the error detected the old value is written with faulty flag
  result.read();
  BOOST_CHECK_EQUAL(result, 11001);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::faulty);

  // -------------------------------------------------------------//
  // recovery from device module exception
  device2DummyBackend->throwExceptionRead = false;
  // When the device recovers, the old value is written with ok flag
  // It might be that the main loop does not write the value each time, so it has
  // to be done once so the data does not stay invalid.
  result.read();
  BOOST_CHECK_EQUAL(result, 11001);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  // finally the loop runs though and propagates the new value
  result.read();
  BOOST_CHECK_EQUAL(result, 11000);
}

BOOST_AUTO_TEST_CASE(testReadDeviceWithTrigger) {
  std::cout << "testReadDeviceWithTrigger" << std::endl;
  return; // FIXME Test does not pass because feature is not implemented yet.
          // See issue #110
  auto trigger = test.getScalar<int>("trigger");
  auto fromDevice = test.getScalar<int>("i3"); // cs side display: m1.i3
  //----------------------------------------------------------------//
  // trigger works as expected
  trigger = 1;

  auto deviceRegister = device1DummyBackend->getRawAccessor("m1", "i3");
  deviceRegister = 30;

  trigger.write();

  CHECK_TIMEOUT(fromDevice.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromDevice, 30);
  BOOST_CHECK(fromDevice.dataValidity() == ctk::DataValidity::ok);

  //----------------------------------------------------------------//
  // Device module exception
  deviceRegister = 10;
  device1DummyBackend->throwExceptionRead = true;

  trigger = 1;
  trigger.write();

  CHECK_TIMEOUT(fromDevice.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromDevice, 30);
  BOOST_CHECK(fromDevice.dataValidity() == ctk::DataValidity::faulty);
  //----------------------------------------------------------------//
  // Recovery
  device1DummyBackend->throwExceptionRead = false;

  trigger = 1;
  trigger.write();

  CHECK_TIMEOUT(fromDevice.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromDevice, 10);
  BOOST_CHECK(fromDevice.dataValidity() == ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_CASE(testConsumingFanout) {
  std::cout << "testConsumingFanout" << std::endl;
  return; // FIXME Test does not pass because feature is not implemented yet.
          // See issue #102
  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto fromConsumingFanout = test.getScalar<int>("m1/i1"); // consumingfanout variable on cs side
  auto result = test.getScalar<int>("m1/Module1_result");

  auto pollRegisterSource = device2DummyBackend->getRawAccessor("m1", "i2");
  pollRegisterSource = 100;

  threadedFanoutInput = 10;

  auto consumingFanoutSource = device1DummyBackend->getRawAccessor("m1", "i1");
  consumingFanoutSource = 1;

  //----------------------------------------------------------//
  // no device module exception
  threadedFanoutInput.write();

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 111);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  CHECK_TIMEOUT(fromConsumingFanout.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromConsumingFanout, 1);
  BOOST_CHECK(fromConsumingFanout.dataValidity() == ctk::DataValidity::ok);

  // --------------------------------------------------------//
  // device exception on consuming fanout source read
  consumingFanoutSource = 0;

  device1DummyBackend->throwExceptionRead = true;
  threadedFanoutInput.write();

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 111);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::faulty);

  CHECK_TIMEOUT(fromConsumingFanout.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromConsumingFanout, 1);
  BOOST_CHECK(fromConsumingFanout.dataValidity() == ctk::DataValidity::faulty);

  // --------------------------------------------------------//
  // Recovery
  device1DummyBackend->throwExceptionRead = true;

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 110);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  CHECK_TIMEOUT(fromConsumingFanout.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromConsumingFanout, 0);
  BOOST_CHECK(fromConsumingFanout.dataValidity() == ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_CASE(testDataFlowOnDeviceException) {
  std::cout << "testDataFlowOnDeviceException" << std::endl;
  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto m1_result = test.getScalar<int>("m1/Module1_result");
  auto m2_result = test.getScalar<int>("m2/Module2_result");

  auto consumingFanoutSource = device1DummyBackend->getRawAccessor("m1", "i1");
  consumingFanoutSource = 1000;

  auto pollRegister = device2DummyBackend->getRawAccessor("m1", "i2");
  pollRegister = 100;

  threadedFanoutInput = 1;

  // ------------------------------------------------------------------//
  // without exception
  threadedFanoutInput.write();
  // Read till the value we want; there is a chance of spurious values
  // sneaking in due to a race condition when dealing with device
  // modules. These spurious entries (with value: PV defaults) do
  // not matter for a real application.
  CHECK_EQUAL_TIMEOUT((m1_result.readNonBlocking(), m1_result), 1101, 10000);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::ok);

  CHECK_EQUAL_TIMEOUT((m2_result.readLatest(), m2_result), 1101, 10000);
  BOOST_CHECK_EQUAL(m2_result, 1101);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::ok);

  // ------------------------------------------------------------------//
  // faulty threadedFanout input
  threadedFanoutInput.setDataValidity(ctk::DataValidity::faulty);
  threadedFanoutInput.write();

  CHECK_TIMEOUT(m1_result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(m1_result, 1101);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::faulty);

  CHECK_TIMEOUT(m2_result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(m2_result, 1101);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::faulty);

  auto deviceStatus =
      test.getScalar<int32_t>(ctk::RegisterPath("/Devices") / TestApplication3::ExceptionDummyCDD2 / "status");
  // the device is still OK
  CHECK_EQUAL_TIMEOUT((deviceStatus.readLatest(), deviceStatus), 0, 10000);

  // ---------------------------------------------------------------------//
  // device module exception
  device2DummyBackend->throwExceptionRead = true;
  pollRegister = 200;
  threadedFanoutInput = 0;
  threadedFanoutInput.write();

  // Now the device has to go into the error state
  CHECK_EQUAL_TIMEOUT((deviceStatus.readLatest(), deviceStatus), 1, 10000);

  // The result of m1 must not be written out again. We can't test this at this safely here.
  // Instead, we know that the next read after recovery will already contain the new data.

  // ---------------------------------------------------------------------//
  // device exception recovery
  device2DummyBackend->throwExceptionRead = false;

  // device error recovers. There must be exactly one new status values with the right value.
  deviceStatus.read();
  BOOST_CHECK(deviceStatus == 0);
  // nothing else in the queue
  BOOST_CHECK(deviceStatus.readNonBlocking() == false);

  m1_result.read(); // we know there must be exaclty one value being written. Wait for it.
  BOOST_CHECK_EQUAL(m1_result, 1200);
  // Data validity still faulty because the input from the fan is invalid
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::faulty);
  // again, nothing else in the queue
  BOOST_CHECK(m1_result.readNonBlocking() == false);

  // same for m2
  m2_result.read();
  BOOST_CHECK_EQUAL(m2_result, 1200);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(m2_result.readNonBlocking() == false);

  // ---------------------------------------------------------------------//
  // recovery: fanout input
  threadedFanoutInput.setDataValidity(ctk::DataValidity::ok);
  threadedFanoutInput.write();

  m1_result.read();
  BOOST_CHECK_EQUAL(m1_result, 1200);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(m1_result.readNonBlocking() == false);

  m2_result.read();
  BOOST_CHECK_EQUAL(m2_result, 1200);
  BOOST_CHECK(m2_result.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(m1_result.readNonBlocking() == false);
}

BOOST_AUTO_TEST_SUITE_END()
