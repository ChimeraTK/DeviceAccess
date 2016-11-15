/*
 * testVariableGroup.cc
 *
 *  Created on: Nov 8, 2016
 *      Author: Martin Hierholzer
 */

#include <future>
#include <chrono>

#define BOOST_TEST_MODULE testVariableGroup

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "VariableGroup.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

struct TestModule : public ctk::ApplicationModule {
    TestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}
  
  struct MixedGroup : public ctk::VariableGroup {
    MixedGroup(ctk::EntityOwner *owner, const std::string &name) : ctk::VariableGroup(owner,name) {}
    CTK_SCALAR_INPUT(int, consumingPush, "MV/m", ctk::UpdateMode::push, "Descrption");
    CTK_SCALAR_INPUT(int, consumingPush2, "MV/m", ctk::UpdateMode::push, "Descrption");
    CTK_SCALAR_INPUT(int, consumingPush3,  "MV/m", ctk::UpdateMode::push, "Descrption");
    CTK_SCALAR_INPUT(int, consumingPoll, "MV/m", ctk::UpdateMode::poll, "Descrption");
    CTK_SCALAR_INPUT(int, consumingPoll2, "MV/m", ctk::UpdateMode::poll, "Descrption");
    CTK_SCALAR_INPUT(int, consumingPoll3, "MV/m", ctk::UpdateMode::poll, "Descrption");
  };
  MixedGroup mixedGroup{this, "mixedGroup"};

  CTK_SCALAR_OUTPUT(int, feedingPush, "MV/m", "Descrption");
  CTK_SCALAR_OUTPUT(int, feedingPush2, "MV/m", "Descrption");
  CTK_SCALAR_OUTPUT(int, feedingPush3,  "MV/m", "Descrption");
  CTK_SCALAR_OUTPUT(int, feedingPoll, "MV/m", "Descrption");
  CTK_SCALAR_OUTPUT(int, feedingPoll2, "MV/m", "Descrption");
  CTK_SCALAR_OUTPUT(int, feedingPoll3, "MV/m", "Descrption");
  
  void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : public ctk::Application {
    TestApplication() : Application("test suite") {}
    ~TestApplication() { shutdown(); }
    
    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests
    
    TestModule testModule{this, "testModule"};
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an app variable */

BOOST_AUTO_TEST_CASE( testMixedGroup ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testMixedGroup" << std::endl;
  
  TestApplication app;
  
  app.testModule.feedingPush >> app.testModule.mixedGroup.consumingPush;
  app.testModule.feedingPush2 >> app.testModule.mixedGroup.consumingPush2;
  app.testModule.feedingPush3 >> app.testModule.mixedGroup.consumingPush3;
  app.testModule.feedingPoll >> app.testModule.mixedGroup.consumingPoll;
  app.testModule.feedingPoll2 >> app.testModule.mixedGroup.consumingPoll2;
  app.testModule.feedingPoll3 >> app.testModule.mixedGroup.consumingPoll3;
  
  app.initialise();
  app.run();
  
  // single theaded test
  app.testModule.feedingPush = 0;
  app.testModule.feedingPush2 = 42;
  app.testModule.feedingPush3 = 120;
  app.testModule.feedingPoll = 10;
  app.testModule.feedingPoll2 = 11;
  app.testModule.feedingPoll3 = 12;
  app.testModule.feedingPoll.write();
  app.testModule.feedingPoll2.write();
  app.testModule.feedingPoll3.write();
  
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 0);

  // test a single write
  app.testModule.feedingPush2.write();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 0);
  app.testModule.mixedGroup.readAny();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);

  // two changes at a time
  app.testModule.feedingPush2 = 666;
  app.testModule.feedingPush2.write();
  app.testModule.feedingPush3.write();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);
  app.testModule.mixedGroup.readAny();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);

  // launch readAny() asynchronously and make sure it does not yet receive anything
  auto futRead = std::async(std::launch::async, [&app]{ app.testModule.mixedGroup.readAny(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);

  // write something  
  app.testModule.feedingPush = 3;
  app.testModule.feedingPush.write();
  
  // check that the group now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);
  
  // launch another readAny() asynchronously and make sure it does not yet receive anything
  auto futRead2 = std::async(std::launch::async, [&app]{ app.testModule.mixedGroup.readAny(); });
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);
  
  // write to the poll-type variables
  app.testModule.feedingPoll = 66;
  app.testModule.feedingPoll.write();
  app.testModule.feedingPoll2 = 77;
  app.testModule.feedingPoll2.write();
  app.testModule.feedingPoll3 = 88;
  app.testModule.feedingPoll3.write();
  
  // make sure readAny still does not receive anything
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);
  
  // write something to the push-type variables
  app.testModule.feedingPush2 = 123;
  app.testModule.feedingPush2.write();
  
  // check that the group now receives the just written values
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 123);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 66);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 77);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 88);
}
