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
#include "ExceptionDevice.h"
#include "ModuleGroup.h"
#include "check_timeout.h"

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
  ctk::ScalarPushInput<int> fromThreadedFanout{ this, "o1", "", "", { "DEVICE1", "CS" } };

  // As a workaround the device side connection is done manually for
  // acheiving this consumingFanout; see:
  // TestApplication3::defineConnections
  ctk::ScalarPollInput<int> fromConsumingFanout{ this, "i1", "", "", { "CS" } };

  ctk::ScalarPollInput<int> fromDevice{ this, "i2", "", "", { "DEVICE2" } };
  ctk::ScalarOutput<int> result{ this, "Module1_result", "", "", { "CS" } };

  void mainLoop() override {
    while (true) {
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
    ctk::ScalarPushInput<int> result{ this,
                                      "Module1_result",
                                      "",
                                      "",
                                      { "CS" } };
  } m1VarsFromCS{ this, "m1", "", ctk::HierarchyModifier::oneLevelUp }; // "m1" being in there 
                                                                        // not good for a general case
  ctk::ScalarOutput<int> result{ this, "Module2_result", "", "", { "CS" } };

  void mainLoop() override {
    while (true) {
      readAll();
      result = static_cast<int>(m1VarsFromCS.result);
      writeAll();
    }
  }
};

// struct TestApplication3 : ctk::ApplicationModule {
struct TestApplication3 : ctk::Application {
/*
 *   CS +---> threaded fanout +------------------+
 *                +                              v
 *                +--------+                   +Device1+
 *                         |                   |       |
 *                         v                +--+       |
 *     CS   <---------+ Module1 <-------+   v          |
 *                 |       ^            +Consuming     |
 *                 |       +---------+    fanout       |
 *                 +----+            +      +          |
 *                      v         Device2   |          |
 *     CS   <---------+ Module2             |          |
 *                                          |          |
 *     CS   <-------------------------------+          |
 *                                                     |
 *                                                     |
 *     CS   <---------+ Trigger <----------------------+
 *                         ^
 *                         |
 *                         +
 *                         CS
 */                        

  constexpr static char const* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=testDataValidity1.map)";
  constexpr static char const* ExceptionDummyCDD2 = "(ExceptionDummy:1?map=testDataValidity2.map)";
  TestApplication3() : Application("testDataFlagPropagation") {}
  ~TestApplication3() { shutdown(); }

  Module1 m1{ this, "m1", "" };
  Module2 m2{ this, "m2", "" };

  ctk::ControlSystemModule cs;
  ctk::DeviceModule device1{ this, ExceptionDummyCDD1 };
  ctk::DeviceModule device2{ this, ExceptionDummyCDD2 };

  void defineConnections() override {
    device1["m1"]("i1") >> m1("i1");
    findTag("CS").connectTo(cs);
    findTag("DEVICE1").connectTo(device1);
    findTag("DEVICE2").connectTo(device2);
    device1["m1"]("i3")[cs("trigger", typeid(int), 1)] >> cs("i3", typeid(int), 1);
  }
};

struct Fixture_testFacility {
  Fixture_testFacility()
      : device1(boost::dynamic_pointer_cast<ExceptionDummy>(
            ChimeraTK::BackendFactory::getInstance().createBackend(
                TestApplication3::ExceptionDummyCDD1))),
        device2(boost::dynamic_pointer_cast<ExceptionDummy>(
            ChimeraTK::BackendFactory::getInstance().createBackend(
                TestApplication3::ExceptionDummyCDD2))) {
    device1->open();
    device2->open();
    test.runApplication();
  }
  boost::shared_ptr<ExceptionDummy> device1;
  boost::shared_ptr<ExceptionDummy> device2;
  TestApplication3 app;
  ctk::TestFacility test;
};

BOOST_FIXTURE_TEST_SUITE(data_validity_propagation, Fixture_testFacility)

BOOST_AUTO_TEST_CASE(testThreadedFanout) {
  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto m1_result = test.getScalar<int>("m1/Module1_result");
  auto m2_result = test.getScalar<int>("m2/Module2_result");

  threadedFanoutInput = 20;
  threadedFanoutInput.write();
  // write to register: m1.i1 linked with the consumingFanout.
  auto consumingFanoutSource = device1->getRawAccessor("m1", "i1");
  consumingFanoutSource = 10;

  auto pollRegister = device2->getRawAccessor("m1", "i2");
  pollRegister = 5;

  test.stepApplication();

  m1_result.read();
  m2_result.read();
  BOOST_CHECK_EQUAL(m1_result, 35);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::ok);

  BOOST_CHECK_EQUAL(m2_result, 35);
  BOOST_CHECK(m2_result.dataValidity()== ctk::DataValidity::ok);

  threadedFanoutInput = 10;
  threadedFanoutInput.setDataValidity(ctk::DataValidity::faulty);
  threadedFanoutInput.write();
  test.stepApplication();

  m1_result.read();
  m2_result.read();
  BOOST_CHECK_EQUAL(m1_result, 25);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(m2_result, 25);
  BOOST_CHECK(m2_result.dataValidity()== ctk::DataValidity::faulty);

  threadedFanoutInput = 40;
  threadedFanoutInput.setDataValidity(ctk::DataValidity::ok);
  threadedFanoutInput.write();
  test.stepApplication();

  m1_result.read();
  m2_result.read();
  BOOST_CHECK_EQUAL(m1_result, 55);
  BOOST_CHECK(m1_result.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(m2_result, 55);
  BOOST_CHECK(m2_result.dataValidity()== ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_CASE(testInvalidTrigger){
  auto deviceRegister = device1->getRawAccessor("m1", "i3");
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

struct Fixture_noTestFacility {
  Fixture_noTestFacility()
      : device1(boost::dynamic_pointer_cast<ExceptionDummy>(
            ChimeraTK::BackendFactory::getInstance().createBackend(
                TestApplication3::ExceptionDummyCDD1))),
        device2(boost::dynamic_pointer_cast<ExceptionDummy>(
            ChimeraTK::BackendFactory::getInstance().createBackend(
                TestApplication3::ExceptionDummyCDD2))) {
    device1->open();
    device2->open();
  }
  boost::shared_ptr<ExceptionDummy> device1;
  boost::shared_ptr<ExceptionDummy> device2;
  TestApplication3 app;
  ctk::TestFacility test{ false };
};

BOOST_FIXTURE_TEST_SUITE(data_validity_propagation_noTestFacility, Fixture_noTestFacility)

BOOST_AUTO_TEST_CASE(testDeviceReadFailure) {
  auto consumingFanoutSource = device1->getRawAccessor("m1", "i1");
  auto pollRegister = device2->getRawAccessor("m1", "i2");

  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto result = test.getScalar<int>("m1/Module1_result");

  threadedFanoutInput = 10000;
  consumingFanoutSource = 1000;
  pollRegister = 1;

  // -------------------------------------------------------------//
  // without errors
  app.run();
  threadedFanoutInput.write();

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 11001);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  // -------------------------------------------------------------//
  // device module exception
  pollRegister = 0;

  device2->throwExceptionRead = true;

  threadedFanoutInput.write();
  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 11001);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::faulty);

  // -------------------------------------------------------------//
  // recovery from device module exception
  device2->throwExceptionRead = false;

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 11000);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_CASE(testreadDeviceWithTrigger) {
  auto trigger = test.getScalar<int>("trigger");
  auto fromDevice = test.getScalar<int>("i3"); // cs side display: m1.i3
  //----------------------------------------------------------------//
  // trigger works as expected
  trigger = 1;

  auto deviceRegister = device1->getRawAccessor("m1", "i3");
  deviceRegister = 30;

  app.run();
  trigger.write();

  CHECK_TIMEOUT(fromDevice.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromDevice, 30);
  BOOST_CHECK(fromDevice.dataValidity() == ctk::DataValidity::ok);

  //----------------------------------------------------------------//
  // Device module exception
  deviceRegister = 10;
  device1->throwExceptionRead = true;

  trigger = 1;
  trigger.write();

  CHECK_TIMEOUT(fromDevice.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromDevice, 30);
  BOOST_CHECK(fromDevice.dataValidity() == ctk::DataValidity::faulty);
  //----------------------------------------------------------------//
  // Recovery
  device1->throwExceptionRead = false;

  trigger = 1;
  trigger.write();

  CHECK_TIMEOUT(fromDevice.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromDevice, 10);
  BOOST_CHECK(fromDevice.dataValidity() == ctk::DataValidity::ok);
}

BOOST_AUTO_TEST_CASE(testConsumingFanout){
  auto threadedFanoutInput = test.getScalar<int>("m1/o1");
  auto fromConsumingFanout = test.getScalar<int>("m1/i1"); // consumingfanout variable on cs side
  auto result = test.getScalar<int>("m1/Module1_result");

  auto pollRegisterSource = device2->getRawAccessor("m1","i2");
  pollRegisterSource = 100; 

  threadedFanoutInput = 10;

  auto consumingFanoutSource = device1->getRawAccessor("m1", "i1");
  consumingFanoutSource = 1;

  //----------------------------------------------------------//
  // no device module exception
  app.run();
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

  device1->throwExceptionRead = true;
  threadedFanoutInput.write();

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 111);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::faulty);

  CHECK_TIMEOUT(fromConsumingFanout.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromConsumingFanout, 1);
  BOOST_CHECK(fromConsumingFanout.dataValidity() == ctk::DataValidity::faulty);

  // --------------------------------------------------------//
  // Recovery
  device1->throwExceptionRead = true;

  CHECK_TIMEOUT(result.readLatest(), 10000);
  BOOST_CHECK_EQUAL(result, 110);
  BOOST_CHECK(result.dataValidity() == ctk::DataValidity::ok);

  CHECK_TIMEOUT(fromConsumingFanout.readLatest(), 10000);
  BOOST_CHECK_EQUAL(fromConsumingFanout, 0);
  BOOST_CHECK(fromConsumingFanout.dataValidity() == ctk::DataValidity::ok);
}
BOOST_AUTO_TEST_SUITE_END()
