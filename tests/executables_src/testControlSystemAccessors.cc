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

#include <ChimeraTK/ControlSystemAdapter/PVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"

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

    CTK_SCALAR_INPUT(T, consumer, "MV/m", ctk::UpdateMode::push, "Descrption");
    CTK_SCALAR_OUTPUT(T, feeder, "MV/m", "Descrption");

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
class TestApplication : public ctk::Application {
  public:
    TestApplication() : Application("test suite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void initialise() {}                    // the setup is done in the tests

    TestModule<T> testModule;
    ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/
/* test feeding a scalar to the control system adapter */

BOOST_AUTO_TEST_CASE_TEMPLATE( testFeedToCS, T, test_types ) {

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.testModule.feeder >> app.cs("myFeeder");
  app.makeConnections();

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 1);
  auto myFeeder = pvManagers.first->getProcessArray<T>("/myFeeder");

  app.testModule.feeder = 42;
  BOOST_CHECK_EQUAL(myFeeder->readNonBlocking(), false);
  app.testModule.feeder.write();
  BOOST_CHECK_EQUAL(myFeeder->readNonBlocking(), true);
  BOOST_CHECK_EQUAL(myFeeder->readNonBlocking(), false);
  BOOST_CHECK(myFeeder->accessData(0) == 42);

  app.testModule.feeder = 120;
  BOOST_CHECK_EQUAL(myFeeder->readNonBlocking(), false);
  app.testModule.feeder.write();
  BOOST_CHECK_EQUAL(myFeeder->readNonBlocking(), true);
  BOOST_CHECK_EQUAL(myFeeder->readNonBlocking(), false);
  BOOST_CHECK(myFeeder->accessData(0) == 120);

}

/*********************************************************************************************************************/
/* test consuming a scalar from the control system adapter */

BOOST_AUTO_TEST_CASE_TEMPLATE( testConsumeFromCS, T, test_types ) {

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.cs("myConsumer") >> app.testModule.consumer;
  app.makeConnections();

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 1);
  auto myConsumer = pvManagers.first->getProcessArray<T>("/myConsumer");

  myConsumer->accessData(0) = 42;
  myConsumer->write();
  app.testModule.consumer.read();
  BOOST_CHECK(app.testModule.consumer == 42);

  myConsumer->accessData(0) = 120;
  myConsumer->write();
  app.testModule.consumer.read();
  BOOST_CHECK(app.testModule.consumer == 120);

}

/*********************************************************************************************************************/
/* test multiple publications of the same variable */

BOOST_AUTO_TEST_CASE_TEMPLATE( testMultiplePublications, T, test_types ) {

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.testModule.feeder >> app.cs("myFeeder0");
  app.testModule.feeder >> app.cs("myFeeder1");
  app.testModule.feeder >> app.cs("myFeeder2");
  app.testModule.feeder >> app.cs("myFeeder3");
  app.run();    // make the connections and start the FanOut threads

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 4);
  auto myFeeder0 = pvManagers.first->getProcessArray<T>("/myFeeder0");
  auto myFeeder1 = pvManagers.first->getProcessArray<T>("/myFeeder1");
  auto myFeeder2 = pvManagers.first->getProcessArray<T>("/myFeeder2");
  auto myFeeder3 = pvManagers.first->getProcessArray<T>("/myFeeder3");

  app.testModule.feeder = 42;
  BOOST_CHECK(myFeeder0->readNonBlocking() == false);
  BOOST_CHECK(myFeeder1->readNonBlocking() == false);
  BOOST_CHECK(myFeeder2->readNonBlocking() == false);
  BOOST_CHECK(myFeeder3->readNonBlocking() == false);
  app.testModule.feeder.write();
  usleep(200000);
  BOOST_CHECK(myFeeder0->readNonBlocking() == true);
  BOOST_CHECK(myFeeder1->readNonBlocking() == true);
  BOOST_CHECK(myFeeder2->readNonBlocking() == true);
  BOOST_CHECK(myFeeder3->readNonBlocking() == true);
  BOOST_CHECK(myFeeder0->accessData(0) == 42);
  BOOST_CHECK(myFeeder1->accessData(0) == 42);
  BOOST_CHECK(myFeeder2->accessData(0) == 42);
  BOOST_CHECK(myFeeder3->accessData(0) == 42);
  BOOST_CHECK(myFeeder0->readNonBlocking() == false);
  BOOST_CHECK(myFeeder1->readNonBlocking() == false);
  BOOST_CHECK(myFeeder2->readNonBlocking() == false);
  BOOST_CHECK(myFeeder3->readNonBlocking() == false);

  app.testModule.feeder = 120;
  BOOST_CHECK(myFeeder0->readNonBlocking() == false);
  BOOST_CHECK(myFeeder1->readNonBlocking() == false);
  BOOST_CHECK(myFeeder2->readNonBlocking() == false);
  BOOST_CHECK(myFeeder3->readNonBlocking() == false);
  app.testModule.feeder.write();
  usleep(200000);
  BOOST_CHECK(myFeeder0->readNonBlocking() == true);
  BOOST_CHECK(myFeeder1->readNonBlocking() == true);
  BOOST_CHECK(myFeeder2->readNonBlocking() == true);
  BOOST_CHECK(myFeeder3->readNonBlocking() == true);
  BOOST_CHECK(myFeeder0->accessData(0) == 120);
  BOOST_CHECK(myFeeder1->accessData(0) == 120);
  BOOST_CHECK(myFeeder2->accessData(0) == 120);
  BOOST_CHECK(myFeeder3->accessData(0) == 120);
  BOOST_CHECK(myFeeder0->readNonBlocking() == false);
  BOOST_CHECK(myFeeder1->readNonBlocking() == false);
  BOOST_CHECK(myFeeder2->readNonBlocking() == false);
  BOOST_CHECK(myFeeder3->readNonBlocking() == false);

  // resend same number
  BOOST_CHECK(myFeeder0->readNonBlocking() == false);
  BOOST_CHECK(myFeeder1->readNonBlocking() == false);
  BOOST_CHECK(myFeeder2->readNonBlocking() == false);
  BOOST_CHECK(myFeeder3->readNonBlocking() == false);
  app.testModule.feeder.write();
  usleep(200000);
  BOOST_CHECK(myFeeder0->readNonBlocking() == true);
  BOOST_CHECK(myFeeder1->readNonBlocking() == true);
  BOOST_CHECK(myFeeder2->readNonBlocking() == true);
  BOOST_CHECK(myFeeder3->readNonBlocking() == true);
  BOOST_CHECK(myFeeder0->accessData(0) == 120);
  BOOST_CHECK(myFeeder1->accessData(0) == 120);
  BOOST_CHECK(myFeeder2->accessData(0) == 120);
  BOOST_CHECK(myFeeder3->accessData(0) == 120);
  BOOST_CHECK(myFeeder0->readNonBlocking() == false);
  BOOST_CHECK(myFeeder1->readNonBlocking() == false);
  BOOST_CHECK(myFeeder2->readNonBlocking() == false);
  BOOST_CHECK(myFeeder3->readNonBlocking() == false);

}

/*********************************************************************************************************************/
/* test multiple re-publications of a variable fed from the control system */

BOOST_AUTO_TEST_CASE_TEMPLATE( testMultipleRePublications, T, test_types ) {

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.cs("myConsumer") >> app.testModule.consumer;
  app.testModule.consumer >> app.cs("myConsumer_copy1");
  app.testModule.consumer >> app.cs("myConsumer_copy2");
  app.testModule.consumer >> app.cs("myConsumer_copy3");
  app.run();    // make the connections and start the FanOut threads

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 4);
  auto myConsumer = pvManagers.first->getProcessArray<T>("/myConsumer");
  auto myConsumer_copy1 = pvManagers.first->getProcessArray<T>("/myConsumer_copy1");
  auto myConsumer_copy2 = pvManagers.first->getProcessArray<T>("/myConsumer_copy2");
  auto myConsumer_copy3 = pvManagers.first->getProcessArray<T>("/myConsumer_copy3");

  myConsumer->accessData(0) = 42;
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == false);
  myConsumer->write();
  usleep(200000);
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy1->accessData(0) == 42);
  BOOST_CHECK(myConsumer_copy2->accessData(0) == 42);
  BOOST_CHECK(myConsumer_copy3->accessData(0) == 42);
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == false);
  app.testModule.consumer.read();
  BOOST_CHECK(app.testModule.consumer == 42);

  myConsumer->accessData(0) = 120;
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == false);
  myConsumer->write();
  usleep(200000);
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy1->accessData(0) == 120);
  BOOST_CHECK(myConsumer_copy2->accessData(0) == 120);
  BOOST_CHECK(myConsumer_copy3->accessData(0) == 120);
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == false);
  app.testModule.consumer.read();
  BOOST_CHECK(app.testModule.consumer == 120);

  // resend same number
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == false);
  myConsumer->write();
  usleep(200000);
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == true);
  BOOST_CHECK(myConsumer_copy1->accessData(0) == 120);
  BOOST_CHECK(myConsumer_copy2->accessData(0) == 120);
  BOOST_CHECK(myConsumer_copy3->accessData(0) == 120);
  BOOST_CHECK(myConsumer_copy1->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy2->readNonBlocking() == false);
  BOOST_CHECK(myConsumer_copy3->readNonBlocking() == false);
  app.testModule.consumer.read();
  BOOST_CHECK(app.testModule.consumer == 120);

}

