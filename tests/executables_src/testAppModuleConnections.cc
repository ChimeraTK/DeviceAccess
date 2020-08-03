/*
 * testAppModuleConnections.cc
 *
 *  Created on: Jun 21, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testAppModuleConnections

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/BackendFactory.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ArrayAccessor.h"
#include "ScalarAccessor.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, float, double> test_types;

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

template<typename T>
struct TestModule : public ctk::ApplicationModule {
  TestModule(EntityOwner* owner, const std::string& name, const std::string& description,
      ctk::HierarchyModifier hierarchyModifier = ctk::HierarchyModifier::none,
      const std::unordered_set<std::string>& tags = {})
  : ApplicationModule(owner, name, description, hierarchyModifier, tags), mainLoopStarted(2) {}

  ctk::ScalarOutput<T> feedingPush{this, "feedingPush", "MV/m", "Some output scalar"};
  ctk::ScalarPushInput<T> consumingPush{this, "consumingPush", "MV/m", "Descrption"};
  ctk::ScalarPushInput<T> consumingPush2{this, "consumingPush2", "MV/m", "Descrption"};
  ctk::ScalarPushInput<T> consumingPush3{this, "consumingPush3", "MV/m", "Descrption"};

  ctk::ScalarPollInput<T> consumingPoll{this, "consumingPoll", "MV/m", "Descrption"};
  ctk::ScalarPollInput<T> consumingPoll2{this, "consumingPoll2", "MV/m", "Descrption"};
  ctk::ScalarPollInput<T> consumingPoll3{this, "consumingPoll3", "MV/m", "Descrption"};

  ctk::ArrayPollInput<T> consumingPollArray{this, "consumingPollArray", "m", 10, "Descrption"};
  ctk::ArrayPushInput<T> consumingPushArray{this, "consumingPushArray", "m", 10, "Descrption"};

  ctk::ArrayOutput<T> feedingArray{this, "feedingArray", "m", 10, "Descrption"};
  ctk::ArrayOutput<T> feedingPseudoArray{this, "feedingPseudoArray", "m", 1, "Descrption"};

  ctk::ScalarPollInput<T> lateConstrScalarPollInput;
  ctk::ScalarPushInput<T> lateConstrScalarPushInput;
  ctk::ScalarOutput<T> lateConstrScalarOutput;

  ctk::ArrayPollInput<T> lateConstrArrayPollInput;
  ctk::ArrayPushInput<T> lateConstrArrayPushInput;
  ctk::ArrayOutput<T> lateConstrArrayOutput;

  // We do not use testable mode for this test, so we need this barrier to synchronise to the beginning of the
  // mainLoop(). This is required since the mainLoopWrapper accesses the module variables before the start of the
  // mainLoop.
  // execute this right after the Application::run():
  //   app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered
  boost::barrier mainLoopStarted;

  void prepare() override {
    incrementDataFaultCounter(); // foce all outputs  to invalid
    writeAll();                  // write initial values
    decrementDataFaultCounter(); // validity according to input validity
  }

  void mainLoop() override { mainLoopStarted.wait(); }
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  using Application::makeConnections; // we call makeConnections() manually in
                                      // the tests to catch exceptions etc.
  void defineConnections() {}         // the setup is done in the tests

  TestModule<T> testModule{this, "testModule", "The test module"};
};

/*********************************************************************************************************************/
/* test case for two scalar accessors in push mode */

BOOST_AUTO_TEST_CASE_TEMPLATE(testTwoScalarPushAccessors, T, test_types) {
  std::cout << "*** testTwoScalarPushAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingPush >> app.testModule.consumingPush;
  app.initialise();
  app.run();
  app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered

  // single theaded test
  app.testModule.consumingPush = 0;
  app.testModule.feedingPush = 42;
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.feedingPush.write();
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.consumingPush.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);

  // launch read() on the consumer asynchronously and make sure it does not yet
  // receive anything
  auto futRead = std::async(std::launch::async, [&app] { app.testModule.consumingPush.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);

  BOOST_CHECK(app.testModule.consumingPush == 42);

  // write to the feeder
  app.testModule.feedingPush = 120;
  app.testModule.feedingPush.write();

  // check that the consumer now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.consumingPush == 120);
}

/*********************************************************************************************************************/
/* test case for four scalar accessors in push mode: one feeder and three
 * consumers */

BOOST_AUTO_TEST_CASE_TEMPLATE(testFourScalarPushAccessors, T, test_types) {
  std::cout << "*** testFourScalarPushAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  // connect in this strange way to test if connection code can handle this.
  app.testModule.consumingPush >> app.testModule.consumingPush2;
  app.testModule.feedingPush >> app.testModule.consumingPush2;
  app.testModule.feedingPush >> app.testModule.consumingPush3;
  app.initialise();
  app.run();
  app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered

  // single theaded test
  app.testModule.consumingPush = 0;
  app.testModule.consumingPush2 = 2;
  app.testModule.consumingPush3 = 3;
  app.testModule.feedingPush = 42;
  BOOST_CHECK(app.testModule.consumingPush == 0);
  BOOST_CHECK(app.testModule.consumingPush2 == 2);
  BOOST_CHECK(app.testModule.consumingPush3 == 3);
  app.testModule.feedingPush.write();
  BOOST_CHECK(app.testModule.consumingPush == 0);
  BOOST_CHECK(app.testModule.consumingPush2 == 2);
  BOOST_CHECK(app.testModule.consumingPush3 == 3);
  app.testModule.consumingPush.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 2);
  BOOST_CHECK(app.testModule.consumingPush3 == 3);
  app.testModule.consumingPush2.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.consumingPush3 == 3);
  app.testModule.consumingPush3.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.consumingPush3 == 42);

  // launch read() on the consumers asynchronously and make sure it does not yet
  // receive anything
  auto futRead = std::async(std::launch::async, [&app] { app.testModule.consumingPush.read(); });
  auto futRead2 = std::async(std::launch::async, [&app] { app.testModule.consumingPush2.read(); });
  auto futRead3 = std::async(std::launch::async, [&app] { app.testModule.consumingPush3.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout);
  BOOST_CHECK(futRead3.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout);

  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.consumingPush3 == 42);

  // write to the feeder
  app.testModule.feedingPush = 120;
  app.testModule.feedingPush.write();

  // check that the consumers now receive the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(futRead3.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.consumingPush == 120);
  BOOST_CHECK(app.testModule.consumingPush2 == 120);
  BOOST_CHECK(app.testModule.consumingPush3 == 120);
}

/*********************************************************************************************************************/
/* test case for two scalar accessors, feeder in push mode and consumer in poll
 * mode */

BOOST_AUTO_TEST_CASE_TEMPLATE(testTwoScalarPushPollAccessors, T, test_types) {
  std::cout << "*** testTwoScalarPushPollAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingPush >> app.testModule.consumingPoll;
  app.initialise();
  app.run();
  app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered

  // single theaded test only, since read() does not block in this case
  app.testModule.consumingPoll = 0;
  app.testModule.feedingPush = 42;
  BOOST_CHECK(app.testModule.consumingPoll == 0);
  app.testModule.feedingPush.write();
  BOOST_CHECK(app.testModule.consumingPoll == 0);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.feedingPush = 120;
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.feedingPush.write();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 120);
}

/*********************************************************************************************************************/
/* test case for two array accessors in push mode */

BOOST_AUTO_TEST_CASE_TEMPLATE(testTwoArrayAccessors, T, test_types) {
  std::cout << "*** testTwoArrayAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingArray >> app.testModule.consumingPushArray;
  app.initialise();
  app.run();
  app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered

  BOOST_CHECK(app.testModule.feedingArray.getNElements() == 10);
  BOOST_CHECK(app.testModule.consumingPushArray.getNElements() == 10);

  // single theaded test
  for(auto& val : app.testModule.consumingPushArray) val = 0;
  for(unsigned int i = 0; i < 10; ++i) app.testModule.feedingArray[i] = 99 + (T)i;
  for(auto& val : app.testModule.consumingPushArray) BOOST_CHECK(val == 0);
  app.testModule.feedingArray.write();
  for(auto& val : app.testModule.consumingPushArray) BOOST_CHECK(val == 0);
  app.testModule.consumingPushArray.read();
  for(unsigned int i = 0; i < 10; ++i) BOOST_CHECK(app.testModule.consumingPushArray[i] == 99 + (T)i);

  // launch read() on the consumer asynchronously and make sure it does not yet
  // receive anything
  auto futRead = std::async(std::launch::async, [&app] { app.testModule.consumingPushArray.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);

  for(unsigned int i = 0; i < 10; ++i) BOOST_CHECK(app.testModule.consumingPushArray[i] == 99 + (T)i);

  // write to the feeder
  for(unsigned int i = 0; i < 10; ++i) app.testModule.feedingArray[i] = 42 - (T)i;
  app.testModule.feedingArray.write();

  // check that the consumer now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  for(unsigned int i = 0; i < 10; ++i) BOOST_CHECK(app.testModule.consumingPushArray[i] == 42 - (T)i);
}

/*********************************************************************************************************************/
/* test case for late constructing accessors */

BOOST_AUTO_TEST_CASE_TEMPLATE(testLateConstruction, T, test_types) {
  std::cout << "*** testLateConstruction<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  // create the scalars
  app.testModule.lateConstrScalarPollInput.replace(ctk::ScalarPollInput<T>(&app.testModule, "LateName1", "", ""));
  app.testModule.lateConstrScalarPushInput.replace(ctk::ScalarPushInput<T>(&app.testModule, "LateName2", "", ""));
  app.testModule.lateConstrScalarOutput.replace(ctk::ScalarOutput<T>(&app.testModule, "LateName3", "", ""));

  // connect the scalars
  app.testModule.lateConstrScalarOutput >> app.testModule.lateConstrScalarPollInput;
  app.testModule.feedingPush >> app.testModule.lateConstrScalarPushInput;

  // create the arrays
  app.testModule.lateConstrArrayPollInput.replace(ctk::ArrayPollInput<T>(&app.testModule, "LateName4", "", 10, ""));
  app.testModule.lateConstrArrayPushInput.replace(ctk::ArrayPushInput<T>(&app.testModule, "LateName5", "", 10, ""));
  app.testModule.lateConstrArrayOutput.replace(ctk::ArrayOutput<T>(&app.testModule, "LateName6", "", 10, ""));

  // connect the arrays
  app.testModule.lateConstrArrayOutput >> app.testModule.lateConstrArrayPollInput;
  app.testModule.feedingArray >> app.testModule.lateConstrArrayPushInput;

  // run the app
  app.initialise();
  app.run();
  app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered

  // test the scalars
  app.testModule.feedingPush = 42;
  app.testModule.feedingPush.write();
  app.testModule.lateConstrScalarPushInput.read();
  BOOST_CHECK(app.testModule.lateConstrScalarPushInput == 42);

  app.testModule.feedingPush = 43;
  app.testModule.feedingPush.write();
  app.testModule.lateConstrScalarPushInput.read();
  BOOST_CHECK(app.testModule.lateConstrScalarPushInput == 43);

  app.testModule.lateConstrScalarOutput = 120;
  app.testModule.lateConstrScalarOutput.write();
  app.testModule.lateConstrScalarPollInput.read();
  BOOST_CHECK_EQUAL(app.testModule.lateConstrScalarPollInput, 120);
  app.testModule.lateConstrScalarPollInput.read();
  BOOST_CHECK_EQUAL(app.testModule.lateConstrScalarPollInput, 120);

  // test the arrays
  app.testModule.feedingArray = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  app.testModule.feedingArray.write();
  app.testModule.lateConstrArrayPushInput.read();
  for(T i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(app.testModule.lateConstrArrayPushInput[i], i + 1);

  app.testModule.feedingArray = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  app.testModule.feedingArray.write();
  app.testModule.lateConstrArrayPushInput.read();
  for(T i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(app.testModule.lateConstrArrayPushInput[i], (i + 1) * 10);

  app.testModule.lateConstrArrayOutput = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  app.testModule.lateConstrArrayOutput.write();
  app.testModule.lateConstrArrayPollInput.read();
  for(T i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(app.testModule.lateConstrArrayPollInput[i], i);
  app.testModule.lateConstrArrayPollInput.read();
  for(T i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(app.testModule.lateConstrArrayPollInput[i], i);
}

/*********************************************************************************************************************/
/* test case for connecting array of length 1 with scalar */

BOOST_AUTO_TEST_CASE_TEMPLATE(testPseudoArray, T, test_types) {
  std::cout << "*** testPseudoArray<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingPseudoArray >> app.testModule.consumingPush;

  // run the app
  app.initialise();
  app.run();
  app.testModule.mainLoopStarted.wait(); // make sure the module's mainLoop() is entered

  // test data transfer
  app.testModule.feedingPseudoArray[0] = 33;
  app.testModule.feedingPseudoArray.write();
  app.testModule.consumingPush.read();
  BOOST_CHECK(app.testModule.consumingPush == 33);
}
