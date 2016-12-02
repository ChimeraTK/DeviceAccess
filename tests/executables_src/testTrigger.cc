/*
 * testTrigger.cc
 *
 *  Created on: Jun 22, 2016
 *      Author: Martin Hierholzer
 */

#include <future>
#include <chrono>

#define BOOST_TEST_MODULE testTrigger

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <mtca4u/BackendFactory.h>
#include <ChimeraTK/ControlSystemAdapter/PVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ControlSystemModule.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t,uint8_t,
                         int16_t,uint16_t,
                         int32_t,uint32_t,
                         float,double>        test_types;

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                   \
    {                                                                                                               \
      std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                  \
      while(!condition) {                                                                                           \
        bool timeout_reached = (std::chrono::steady_clock::now()-t0) > std::chrono::milliseconds(maxMilliseconds);  \
        BOOST_CHECK( !timeout_reached );                                                                            \
        if(timeout_reached) break;                                                                                  \
        usleep(1000);                                                                                               \
      }                                                                                                             \
    }

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

template<typename T>
struct TestModule : public ctk::ApplicationModule {
    TestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    ctk::ScalarPushInput<T> consumingPush{this, "consumingPush", "MV/m", "Descrption"};
    ctk::ScalarPushInput<T> consumingPush2{this, "consumingPush2", "MV/m", "Descrption"};
    ctk::ScalarPushInput<T> consumingPush3{this, "consumingPush3", "MV/m", "Descrption"};

    ctk::ScalarPollInput<T> consumingPoll{this, "consumingPoll", "MV/m", "Descrption"};
    ctk::ScalarPollInput<T> consumingPoll2{this, "consumingPoll2", "MV/m", "Descrption"};
    ctk::ScalarPollInput<T> consumingPoll3{this, "consumingPoll3", "MV/m", "Descrption"};

    ctk::ScalarOutput<T> theTrigger{this, "theTrigger", "MV/m", "Descrption"};
    ctk::ScalarOutput<T> feedingToDevice{this, "feedingToDevice", "MV/m", "Descrption"};

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

    TestModule<T> testModule{this,"testModule"};
    ctk::DeviceModule dev{"Dummy0"};
    ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an app variable */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTriggerDevToApp, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testTriggerDevToApp<" << typeid(T).name() << ">" << std::endl;

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication<T> app;

  app.testModule.feedingToDevice >> app.dev("/MyModule/Variable");

  app.dev("/MyModule/Variable") [ app.testModule.theTrigger ] >> app.testModule.consumingPush;
  app.initialise();
  app.run();

  // single theaded test
  app.testModule.consumingPush = 0;
  app.testModule.feedingToDevice = 42;
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.feedingToDevice.write();
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.theTrigger.write();
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.consumingPush.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);

  // launch read() on the consumer asynchronously and make sure it does not yet receive anything
  auto futRead = std::async(std::launch::async, [&app]{ app.testModule.consumingPush.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);

  BOOST_CHECK(app.testModule.consumingPush == 42);

  // write to the feeder
  app.testModule.feedingToDevice = 120;
  app.testModule.feedingToDevice.write();
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(app.testModule.consumingPush == 42);

  // send trigger
  app.testModule.theTrigger.write();

  // check that the consumer now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK( app.testModule.consumingPush == 120 );
}

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to control system variable */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTriggerDevToCS, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testTriggerDevToCS<" << typeid(T).name() << ">" << std::endl;

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.testModule.feedingToDevice >> app.dev("/MyModule/Variable");

  app.dev("/MyModule/Variable", typeid(T), 1) [ app.testModule.theTrigger ] >> app.cs("myCSVar");
  
  app.initialise();
  app.run();

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 1);
  auto myCSVar = pvManagers.first->getProcessArray<T>("/myCSVar");

  // single theaded test only, since the receiving process scalar does not support blocking
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice = 42;
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice.write();
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.theTrigger.write();
  CHECK_TIMEOUT(myCSVar->readNonBlocking() == true, 30000);
  BOOST_CHECK(myCSVar->accessData(0) == 42);

  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice = 120;
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice.write();
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.theTrigger.write();
  CHECK_TIMEOUT(myCSVar->readNonBlocking() == true, 30000);
  BOOST_CHECK(myCSVar->accessData(0) == 120);

  BOOST_CHECK(myCSVar->readNonBlocking() == false);
}

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to control system variable */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTriggerByCS, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testTriggerByCS<" << typeid(T).name() << ">" << std::endl;

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.testModule.feedingToDevice >> app.dev("/MyModule/Variable");

  app.dev("/MyModule/Variable", typeid(T), 1) [ app.cs("theTrigger", typeid(T), 1) ] >> app.cs("myCSVar");
  
  app.initialise();
  app.run();

  BOOST_CHECK_EQUAL(pvManagers.first->getAllProcessVariables().size(), 2);
  auto myCSVar = pvManagers.first->getProcessArray<T>("/myCSVar");
  auto theTrigger = pvManagers.first->getProcessArray<T>("/theTrigger");
  
  // single theaded test only, since the receiving process scalar does not support blocking
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice = 42;
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice.write();
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  myCSVar->accessData(0) = 0;
  theTrigger->write();
  CHECK_TIMEOUT(myCSVar->readNonBlocking() == true, 30000);
  BOOST_CHECK(myCSVar->accessData(0) == 42);

  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice = 120;
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  app.testModule.feedingToDevice.write();
  BOOST_CHECK(myCSVar->readNonBlocking() == false);
  myCSVar->accessData(0) = 0;
  theTrigger->write();
  CHECK_TIMEOUT(myCSVar->readNonBlocking() == true, 30000);
  BOOST_CHECK(myCSVar->accessData(0) == 120);

  BOOST_CHECK(myCSVar->readNonBlocking() == false);
}
