/*
 * testTrigger.cc
 *
 *  Created on: Jun 22, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testTrigger

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <mtca4u/BackendFactory.h>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t,uint8_t,
                         int16_t,uint16_t,
                         int32_t,uint32_t,
                         float,double>        test_types;

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

template<typename T>
class TestModule : public ctk::ApplicationModule {
  public:
    SCALAR_INPUT(T, consumingPush, "MV/m", ctk::UpdateMode::push);
    SCALAR_INPUT(T, consumingPush2, "MV/m", ctk::UpdateMode::push);
    SCALAR_INPUT(T, consumingPush3,  "MV/m", ctk::UpdateMode::push);

    SCALAR_INPUT(T, consumingPoll, "MV/m", ctk::UpdateMode::poll);
    SCALAR_INPUT(T, consumingPoll2, "MV/m", ctk::UpdateMode::poll);
    SCALAR_INPUT(T, consumingPoll3, "MV/m", ctk::UpdateMode::poll);

    SCALAR_OUTPUT(T, theTrigger, "MV/m");
    SCALAR_OUTPUT(T, feedingToDevice, "MV/m");

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

class TestApplication : public ctk::Application {
  public:
    using Application::Application;
    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void initialise() {}                    // the setup is done in the tests
};

/*********************************************************************************************************************/
/* test trigger used when connecting a polled feeder to a pushed consumer */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTriggerPollFeederPushConsumer, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication app("Test Suite");
  TestModule<T> testModule;

  testModule.feedingToDevice.feedToDevice("Dummy0","/MyModule/Variable");

  testModule.consumingPush.consumeFromDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
  testModule.consumingPush.addTrigger(testModule.theTrigger);
  app.run();

  // single theaded test
  testModule.consumingPush = 0;
  testModule.feedingToDevice = 42;
  BOOST_CHECK(testModule.consumingPush == 0);
  testModule.feedingToDevice.write();
  testModule.theTrigger.write();
  BOOST_CHECK(testModule.consumingPush == 0);
  testModule.consumingPush.read();
  BOOST_CHECK(testModule.consumingPush == 42);

  // launch read() on the consumer asynchronously and make sure it does not yet receive anything
  auto futRead = std::async(std::launch::async, [&testModule]{ testModule.consumingPush.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);

  BOOST_CHECK(testModule.consumingPush == 42);

  // write to the feeder
  testModule.feedingToDevice = 120;
  testModule.feedingToDevice.write();
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(testModule.consumingPush == 42);

  // send trigger
  testModule.theTrigger.write();

  // check that the consumer now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::ready);
  BOOST_CHECK( testModule.consumingPush == 120 );
}
