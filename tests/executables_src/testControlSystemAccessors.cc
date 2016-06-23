/*
 * testControlSystemAccessors.cc
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testControlSystemAccessors

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <ControlSystemAdapter/PVManager.h>
#include <ControlSystemAdapter/ControlSystemPVManager.h>
#include <ControlSystemAdapter/DevicePVManager.h>

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

    SCALAR_INPUT(T, consumer, "MV/m", ctk::UpdateMode::push);
    SCALAR_OUTPUT(T, feeder, "MV/m");

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
/* test feeding a scalar to the control system adapter */

BOOST_AUTO_TEST_CASE_TEMPLATE( testFeedToCS, T, test_types ) {

  TestApplication app("Test Suite");
  TestModule<T> testModule;

  auto pvManagers = mtca4u::createPVManager();
  app.setPVManager(pvManagers.second);

  testModule.feeder.feedToControlSystem("myFeeder");
  app.makeConnections();

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 1);
  auto myFeeder = pvManagers.first->getProcessScalar<T>("myFeeder");

  testModule.feeder = 42;
  BOOST_CHECK_EQUAL(myFeeder->receive(), false);
  testModule.feeder.write();
  BOOST_CHECK_EQUAL(myFeeder->receive(), true);
  BOOST_CHECK_EQUAL(myFeeder->receive(), false);
  BOOST_CHECK(*myFeeder == 42);

  testModule.feeder = 120;
  BOOST_CHECK_EQUAL(myFeeder->receive(), false);
  testModule.feeder.write();
  BOOST_CHECK_EQUAL(myFeeder->receive(), true);
  BOOST_CHECK_EQUAL(myFeeder->receive(), false);
  BOOST_CHECK(*myFeeder == 120);

}

/*********************************************************************************************************************/
/*test consuming a scalar from the control system adapter */

BOOST_AUTO_TEST_CASE_TEMPLATE( testConsumeFromCS, T, test_types ) {

  TestApplication app("Test Suite");
  TestModule<T> testModule;

  auto pvManagers = mtca4u::createPVManager();
  app.setPVManager(pvManagers.second);

  testModule.consumer.consumeFromControlSystem("myConsumer");
  app.makeConnections();

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 1);
  auto myConsumer = pvManagers.first->getProcessScalar<T>("myConsumer");

  *myConsumer = 42;
  myConsumer->send();
  testModule.consumer.read();
  BOOST_CHECK(testModule.consumer == 42);

  *myConsumer = 120;
  myConsumer->send();
  testModule.consumer.read();
  BOOST_CHECK(testModule.consumer == 120);

}

