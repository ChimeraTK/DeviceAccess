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

#include "Application.h"
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
struct TestModule : public ctk::ApplicationModule {
    TestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    ctk::ScalarOutput<T> feedingPush{this, "feedingPush", "MV/m", "Descrption"};
    ctk::ScalarOutput<T> feedingPush2{this, "feedingPush2", "MV/m", "Descrption"};
    ctk::ScalarPushInput<T> consumingPush{this, "consumingPush", "MV/m", "Descrption"};
    ctk::ScalarPushInput<T> consumingPush2{this, "consumingPush2", "MV/m", "Descrption"};
    ctk::ScalarPushInput<T> consumingPush3{this, "consumingPush3", "MV/m", "Descrption"};

    ctk::ScalarPollInput<T> consumingPoll{this, "consumingPoll", "MV/m", "Descrption"};
    ctk::ScalarPollInput<T> consumingPoll2{this, "consumingPoll2", "MV/m", "Descrption"};
    ctk::ScalarPollInput<T> consumingPoll3{this, "consumingPoll3", "MV/m", "Descrption"};

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
    TestApplication() : Application("test suite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests

    TestModule<T> testModule{this, "testModule"};
    ctk::DeviceModule dev{"Dummy0"};
};

/*********************************************************************************************************************/
/* test case for two scalar accessors, feeder in poll mode and consumer in push mode (without trigger) */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoScalarPollPushAccessors, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");
  TestApplication<T> app;

  app.dev("/MyModule/Variable") >> app.testModule.consumingPush;
  try {
    app.initialise();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
    BOOST_CHECK_NO_THROW( e.what(); );
  }

}

/*********************************************************************************************************************/
/* test case for no feeder */

BOOST_AUTO_TEST_CASE_TEMPLATE( testNoFeeder, T, test_types ) {

  TestApplication<T> app;

  app.testModule.consumingPush2 >> app.testModule.consumingPush;
  try {
    app.initialise();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
    BOOST_CHECK_NO_THROW( e.what(); );
  }

}

/*********************************************************************************************************************/
/* test case for two feeders */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoFeeders, T, test_types ) {

  TestApplication<T> app;

  try {
    app.testModule.feedingPush >> app.testModule.feedingPush2;
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
    BOOST_CHECK_NO_THROW( e.what(); );
  }

}

/*********************************************************************************************************************/
/* test case for too many polling consumers */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTooManyPollingConsumers, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication<T> app;

  app.dev("/MyModule/Variable") >> app.testModule.consumingPoll >> app.testModule.consumingPoll2;
  try {
    app.initialise();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
    BOOST_CHECK_NO_THROW( e.what(); );
  }

}
