/*
 * testIllegalNetworks.cc
 *
 *  Created on: Jun 21, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testIllegalNetworks

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <mtca4u/BackendFactory.h>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"

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
    SCALAR_OUTPUT(T, feedingPush, "MV/m");
    SCALAR_OUTPUT(T, feedingPush2, "MV/m");
    SCALAR_INPUT(T, consumingPush, "MV/m", ctk::UpdateMode::push);
    SCALAR_INPUT(T, consumingPush2, "MV/m", ctk::UpdateMode::push);
    SCALAR_INPUT(T, consumingPush3,  "MV/m", ctk::UpdateMode::push);

    SCALAR_INPUT(T, consumingPoll, "MV/m", ctk::UpdateMode::poll);
    SCALAR_INPUT(T, consumingPoll2, "MV/m", ctk::UpdateMode::poll);
    SCALAR_INPUT(T, consumingPoll3, "MV/m", ctk::UpdateMode::poll);

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

class TestApplication : public ctk::Application {
  public:
    TestApplication() : Application("test suite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void initialise() {}                    // the setup is done in the tests
};

/*********************************************************************************************************************/
/* test case for two scalar accessors, feeder in poll mode and consumer in push mode (without trigger) */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoScalarPollPushAccessors, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication app;
  TestModule<T> testModule;

  ctk::DeviceModule dev{"Dummy0"};

  dev("/MyModule/Variable") >> testModule.consumingPush;
  try {
    app.makeConnections();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
  }

}

/*********************************************************************************************************************/
/* test case for no feeder */

BOOST_AUTO_TEST_CASE_TEMPLATE( testNoFeeder, T, test_types ) {

  TestApplication app;
  TestModule<T> testModule;

  testModule.consumingPush2 >> testModule.consumingPush;
  try {
    app.makeConnections();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
  }

}

/*********************************************************************************************************************/
/* test case for two feeders */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoFeeders, T, test_types ) {

  TestApplication app;
  TestModule<T> testModule;

  try {
    testModule.feedingPush >> testModule.feedingPush2;
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
  }

}

/*********************************************************************************************************************/
/* test case for too many polling consumers */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTooManyPollingConsumers, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication app;
  TestModule<T> testModule;

  ctk::DeviceModule dev{"Dummy0"};

  dev("/MyModule/Variable") >> testModule.consumingPoll >> testModule.consumingPoll2;
  try {
    app.makeConnections();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
  }

}
