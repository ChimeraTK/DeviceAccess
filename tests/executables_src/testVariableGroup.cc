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
#include <boost/thread.hpp>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "VariableGroup.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

struct TestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  struct MixedGroup : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<int> consumingPush{this, "consumingPush", "MV/m", "Descrption"};
    ctk::ScalarPushInput<int> consumingPush2{this, "consumingPush2", "MV/m", "Descrption"};
    ctk::ScalarPushInput<int> consumingPush3{this, "consumingPush3", "MV/m", "Descrption"};
    ctk::ScalarPollInput<int> consumingPoll{this, "consumingPoll", "MV/m", "Descrption"};
    ctk::ScalarPollInput<int> consumingPoll2{this, "consumingPoll2", "MV/m", "Descrption"};
    ctk::ScalarPollInput<int> consumingPoll3{this, "consumingPoll3", "MV/m", "Descrption"};
  };
  MixedGroup mixedGroup{this, "mixedGroup", "A group with both push and poll inputs"};

  ctk::ScalarOutput<int> feedingPush{this, "feedingPush", "MV/m", "Descrption"};
  ctk::ScalarOutput<int> feedingPush2{this, "feedingPush2", "MV/m", "Descrption"};
  ctk::ScalarOutput<int> feedingPush3{this, "feedingPush3", "MV/m", "Descrption"};
  ctk::ScalarOutput<int> feedingPoll{this, "feedingPoll", "MV/m", "Descrption"};
  ctk::ScalarOutput<int> feedingPoll2{this, "feedingPoll2", "MV/m", "Descrption"};
  ctk::ScalarOutput<int> feedingPoll3{this, "feedingPoll3", "MV/m", "Descrption"};

  void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : public ctk::Application {
    TestApplication() : Application("testSuite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests

    TestModule testModule{this, "testModule", "The test module"};
};

/*********************************************************************************************************************/
/* test module-wide read/write operations */

BOOST_AUTO_TEST_CASE( testModuleReadWrite ) {
  std::cout << "*** testModuleReadWrite" << std::endl;

  TestApplication app;

  app.testModule.feedingPush >> app.testModule.mixedGroup.consumingPush;
  app.testModule.feedingPush2 >> app.testModule.mixedGroup.consumingPush2;
  app.testModule.feedingPush3 >> app.testModule.mixedGroup.consumingPush3;
  app.testModule.feedingPoll >> app.testModule.mixedGroup.consumingPoll;
  app.testModule.feedingPoll2 >> app.testModule.mixedGroup.consumingPoll2;
  app.testModule.feedingPoll3 >> app.testModule.mixedGroup.consumingPoll3;
  app.initialise();

  // single theaded test
  app.testModule.mixedGroup.consumingPush = 666;
  app.testModule.mixedGroup.consumingPush2 = 666;
  app.testModule.mixedGroup.consumingPush3 = 666;
  app.testModule.mixedGroup.consumingPoll = 666;
  app.testModule.mixedGroup.consumingPoll2 = 666;
  app.testModule.mixedGroup.consumingPoll3 = 666;
  app.testModule.feedingPush = 18;
  app.testModule.feedingPush2 = 20;
  app.testModule.feedingPush3 = 22;
  app.testModule.feedingPoll = 23;
  app.testModule.feedingPoll2 = 24;
  app.testModule.feedingPoll3 = 27;
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 666);
  app.testModule.writeAll();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 666);
  app.testModule.readAll();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 18);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 20);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 24);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 27);

  app.testModule.readAllNonBlocking();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 18);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 20);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 24);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 27);

  app.testModule.feedingPush2 = 30;
  app.testModule.feedingPoll2 = 33;
  app.testModule.writeAll();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 18);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 20);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 24);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 27);
  app.testModule.readAllNonBlocking();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 18);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 27);
  app.testModule.readAllNonBlocking();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 18);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 27);

  app.testModule.feedingPush = 35;
  app.testModule.feedingPoll3 = 40;
  app.testModule.writeAll();
  app.testModule.feedingPush = 36;
  app.testModule.feedingPoll3 = 44;
  app.testModule.writeAll();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 18);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 27);
  app.testModule.readAllNonBlocking();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 35);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 44);
  app.testModule.readAllNonBlocking();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 36);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 44);
  app.testModule.readAllNonBlocking();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 36);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 44);

  app.testModule.feedingPush = 45;
  app.testModule.writeAll();
  app.testModule.feedingPush = 46;
  app.testModule.writeAll();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 36);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 44);
  app.testModule.readAllLatest();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 46);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 44);
  app.testModule.readAllLatest();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 46);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 30);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 22);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 23);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 33);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 44);

}

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an app variable */

BOOST_AUTO_TEST_CASE( testReadAny ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testReadAny" << std::endl;

  TestApplication app;

  app.testModule.feedingPush >> app.testModule.mixedGroup.consumingPush;
  app.testModule.feedingPush2 >> app.testModule.mixedGroup.consumingPush2;
  app.testModule.feedingPush3 >> app.testModule.mixedGroup.consumingPush3;
  app.testModule.feedingPoll >> app.testModule.mixedGroup.consumingPoll;
  app.testModule.feedingPoll2 >> app.testModule.mixedGroup.consumingPoll2;
  app.testModule.feedingPoll3 >> app.testModule.mixedGroup.consumingPoll3;

  app.initialise();
  app.run();

  auto group = app.testModule.mixedGroup.readAnyGroup();

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
  group.readAny();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);

  // two more writes
  app.testModule.feedingPush2 = 666;
  app.testModule.feedingPush2.write();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);
  group.readAny();
  app.testModule.feedingPush3.write();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);
  group.readAny();
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 0);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);

  // launch readAny() asynchronously and make sure it does not yet receive anything
  auto futureRead = std::async(std::launch::async, [&group]{ group.readAny(); });
  BOOST_CHECK(futureRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
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
  BOOST_CHECK(futureRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 666);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 10);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 11);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 12);

  // launch another readAny() asynchronously and make sure it does not yet receive anything
  auto futureRead2 = std::async(std::launch::async, [&group]{ group.readAny(); });
  BOOST_CHECK(futureRead2.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
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
  BOOST_CHECK(futureRead2.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
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
  BOOST_CHECK(futureRead2.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 123);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 66);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 77);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 88);

  // two changes at a time
  auto futureRead3 = std::async(std::launch::async, [&group]{ group.readAny(); });
  BOOST_CHECK(futureRead3.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 123);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 120);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 66);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 77);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 88);

  app.testModule.feedingPush2 = 234;
  app.testModule.feedingPush3 = 345;
  app.testModule.feedingPush2.write();
  app.testModule.feedingPush3.write();

  BOOST_CHECK(futureRead3.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  auto futureRead4 = std::async(std::launch::async, [&group]{ group.readAny(); });
  BOOST_CHECK(futureRead4.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush == 3);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush2 == 234);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPush3 == 345);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll == 66);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll2 == 77);
  BOOST_CHECK(app.testModule.mixedGroup.consumingPoll3 == 88);

}
