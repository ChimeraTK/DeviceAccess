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
/* test consuming a scalar from the control system adapter */

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

/*********************************************************************************************************************/
/* test multiple publications of the same variable */

BOOST_AUTO_TEST_CASE_TEMPLATE( testMultiplePublications, T, test_types ) {

  TestApplication app("Test Suite");
  TestModule<T> testModule;

  auto pvManagers = mtca4u::createPVManager();
  app.setPVManager(pvManagers.second);

  testModule.feeder.feedToControlSystem("myFeeder0");
  testModule.feeder.feedToControlSystem("myFeeder1");
  testModule.feeder.feedToControlSystem("myFeeder2");
  testModule.feeder.feedToControlSystem("myFeeder3");
  app.run();    // make the connections and start the FanOut threads

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 4);
  auto myFeeder0 = pvManagers.first->getProcessScalar<T>("myFeeder0");
  auto myFeeder1 = pvManagers.first->getProcessScalar<T>("myFeeder1");
  auto myFeeder2 = pvManagers.first->getProcessScalar<T>("myFeeder2");
  auto myFeeder3 = pvManagers.first->getProcessScalar<T>("myFeeder3");

  testModule.feeder = 42;
  BOOST_CHECK(myFeeder0->receive() == false);
  BOOST_CHECK(myFeeder1->receive() == false);
  BOOST_CHECK(myFeeder2->receive() == false);
  BOOST_CHECK(myFeeder3->receive() == false);
  testModule.feeder.write();
  usleep(200000);
  BOOST_CHECK(myFeeder0->receive() == true);
  BOOST_CHECK(myFeeder1->receive() == true);
  BOOST_CHECK(myFeeder2->receive() == true);
  BOOST_CHECK(myFeeder3->receive() == true);
  BOOST_CHECK(*myFeeder0 == 42);
  BOOST_CHECK(*myFeeder1 == 42);
  BOOST_CHECK(*myFeeder2 == 42);
  BOOST_CHECK(*myFeeder3 == 42);
  BOOST_CHECK(myFeeder0->receive() == false);
  BOOST_CHECK(myFeeder1->receive() == false);
  BOOST_CHECK(myFeeder2->receive() == false);
  BOOST_CHECK(myFeeder3->receive() == false);

  testModule.feeder = 120;
  BOOST_CHECK(myFeeder0->receive() == false);
  BOOST_CHECK(myFeeder1->receive() == false);
  BOOST_CHECK(myFeeder2->receive() == false);
  BOOST_CHECK(myFeeder3->receive() == false);
  testModule.feeder.write();
  usleep(200000);
  BOOST_CHECK(myFeeder0->receive() == true);
  BOOST_CHECK(myFeeder1->receive() == true);
  BOOST_CHECK(myFeeder2->receive() == true);
  BOOST_CHECK(myFeeder3->receive() == true);
  BOOST_CHECK(*myFeeder0 == 120);
  BOOST_CHECK(*myFeeder1 == 120);
  BOOST_CHECK(*myFeeder2 == 120);
  BOOST_CHECK(*myFeeder3 == 120);
  BOOST_CHECK(myFeeder0->receive() == false);
  BOOST_CHECK(myFeeder1->receive() == false);
  BOOST_CHECK(myFeeder2->receive() == false);
  BOOST_CHECK(myFeeder3->receive() == false);

  // resend same number
  BOOST_CHECK(myFeeder0->receive() == false);
  BOOST_CHECK(myFeeder1->receive() == false);
  BOOST_CHECK(myFeeder2->receive() == false);
  BOOST_CHECK(myFeeder3->receive() == false);
  testModule.feeder.write();
  usleep(200000);
  BOOST_CHECK(myFeeder0->receive() == true);
  BOOST_CHECK(myFeeder1->receive() == true);
  BOOST_CHECK(myFeeder2->receive() == true);
  BOOST_CHECK(myFeeder3->receive() == true);
  BOOST_CHECK(*myFeeder0 == 120);
  BOOST_CHECK(*myFeeder1 == 120);
  BOOST_CHECK(*myFeeder2 == 120);
  BOOST_CHECK(*myFeeder3 == 120);
  BOOST_CHECK(myFeeder0->receive() == false);
  BOOST_CHECK(myFeeder1->receive() == false);
  BOOST_CHECK(myFeeder2->receive() == false);
  BOOST_CHECK(myFeeder3->receive() == false);

}

/*********************************************************************************************************************/
/* test multiple re-publications of a variable fed from teh control system */

BOOST_AUTO_TEST_CASE_TEMPLATE( testMultipleRePublications, T, test_types ) {

  TestApplication app("Test Suite");
  TestModule<T> testModule;

  auto pvManagers = mtca4u::createPVManager();
  app.setPVManager(pvManagers.second);

  testModule.consumer.consumeFromControlSystem("myConsumer");
  testModule.consumer.feedToControlSystem("myConsumer_copy1");
  testModule.consumer.feedToControlSystem("myConsumer_copy2");
  testModule.consumer.feedToControlSystem("myConsumer_copy3");
  app.run();    // make the connections and start the FanOut threads

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 4);
  auto myConsumer = pvManagers.first->getProcessScalar<T>("myConsumer");
  auto myConsumer_copy1 = pvManagers.first->getProcessScalar<T>("myConsumer_copy1");
  auto myConsumer_copy2 = pvManagers.first->getProcessScalar<T>("myConsumer_copy2");
  auto myConsumer_copy3 = pvManagers.first->getProcessScalar<T>("myConsumer_copy3");

  *myConsumer = 42;
  BOOST_CHECK(myConsumer_copy1->receive() == false);
  BOOST_CHECK(myConsumer_copy2->receive() == false);
  BOOST_CHECK(myConsumer_copy3->receive() == false);
  myConsumer->send();
  usleep(200000);
  BOOST_CHECK(myConsumer_copy1->receive() == true);
  BOOST_CHECK(myConsumer_copy2->receive() == true);
  BOOST_CHECK(myConsumer_copy3->receive() == true);
  BOOST_CHECK(*myConsumer_copy1 == 42);
  BOOST_CHECK(*myConsumer_copy2 == 42);
  BOOST_CHECK(*myConsumer_copy3 == 42);
  BOOST_CHECK(myConsumer_copy1->receive() == false);
  BOOST_CHECK(myConsumer_copy2->receive() == false);
  BOOST_CHECK(myConsumer_copy3->receive() == false);
  testModule.consumer.read();
  BOOST_CHECK(testModule.consumer == 42);

  *myConsumer = 120;
  BOOST_CHECK(myConsumer_copy1->receive() == false);
  BOOST_CHECK(myConsumer_copy2->receive() == false);
  BOOST_CHECK(myConsumer_copy3->receive() == false);
  myConsumer->send();
  usleep(200000);
  BOOST_CHECK(myConsumer_copy1->receive() == true);
  BOOST_CHECK(myConsumer_copy2->receive() == true);
  BOOST_CHECK(myConsumer_copy3->receive() == true);
  BOOST_CHECK(*myConsumer_copy1 == 120);
  BOOST_CHECK(*myConsumer_copy2 == 120);
  BOOST_CHECK(*myConsumer_copy3 == 120);
  BOOST_CHECK(myConsumer_copy1->receive() == false);
  BOOST_CHECK(myConsumer_copy2->receive() == false);
  BOOST_CHECK(myConsumer_copy3->receive() == false);
  testModule.consumer.read();
  BOOST_CHECK(testModule.consumer == 120);

  // resend same number
  BOOST_CHECK(myConsumer_copy1->receive() == false);
  BOOST_CHECK(myConsumer_copy2->receive() == false);
  BOOST_CHECK(myConsumer_copy3->receive() == false);
  myConsumer->send();
  usleep(200000);
  BOOST_CHECK(myConsumer_copy1->receive() == true);
  BOOST_CHECK(myConsumer_copy2->receive() == true);
  BOOST_CHECK(myConsumer_copy3->receive() == true);
  BOOST_CHECK(*myConsumer_copy1 == 120);
  BOOST_CHECK(*myConsumer_copy2 == 120);
  BOOST_CHECK(*myConsumer_copy3 == 120);
  BOOST_CHECK(myConsumer_copy1->receive() == false);
  BOOST_CHECK(myConsumer_copy2->receive() == false);
  BOOST_CHECK(myConsumer_copy3->receive() == false);
  testModule.consumer.read();
  BOOST_CHECK(testModule.consumer == 120);

}

