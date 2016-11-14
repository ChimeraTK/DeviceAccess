/*
 * testAppModuleConnections.cc
 *
 *  Created on: Jun 21, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testAppModuleConnections

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <mtca4u/BackendFactory.h>

#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
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
    CTK_SCALAR_OUTPUT(T, feedingPush, "MV/m", "Some output scalar");
    CTK_SCALAR_INPUT(T, consumingPush, "MV/m", ctk::UpdateMode::push, "Descrption");
    CTK_SCALAR_INPUT(T, consumingPush2, "MV/m", ctk::UpdateMode::push, "Descrption");
    CTK_SCALAR_INPUT(T, consumingPush3,  "MV/m", ctk::UpdateMode::push, "Descrption");

    CTK_SCALAR_INPUT(T, consumingPoll, "MV/m", ctk::UpdateMode::poll, "Descrption");
    CTK_SCALAR_INPUT(T, consumingPoll2, "MV/m", ctk::UpdateMode::poll, "Descrption");
    CTK_SCALAR_INPUT(T, consumingPoll3, "MV/m", ctk::UpdateMode::poll, "Descrption");
    
    CTK_ARRAY_INPUT(T, consumingPollArray, "m", 10, ctk::UpdateMode::poll, "Descrption");
    CTK_ARRAY_INPUT(T, consumingPushArray, "m", 10, ctk::UpdateMode::push, "Descrption");
    
    CTK_ARRAY_OUTPUT(T, feedingArray, "m", 10, "Descrption");

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
    void defineConnections() {}             // the setup is done in the tests

    TestModule<T> testModule;
};

/*********************************************************************************************************************/
/* test case for two scalar accessors in push mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoScalarPushAccessors, T, test_types ) {
  std::cout << "*** testTwoScalarPushAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingPush >> app.testModule.consumingPush;
  app.initialise();

  // single theaded test
  app.testModule.consumingPush = 0;
  app.testModule.feedingPush = 42;
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.feedingPush.write();
  BOOST_CHECK(app.testModule.consumingPush == 0);
  app.testModule.consumingPush.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);

  // launch read() on the consumer asynchronously and make sure it does not yet receive anything
  auto futRead = std::async(std::launch::async, [&app]{ app.testModule.consumingPush.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);

  BOOST_CHECK(app.testModule.consumingPush == 42);

  // write to the feeder
  app.testModule.feedingPush = 120;
  app.testModule.feedingPush.write();

  // check that the consumer now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK( app.testModule.consumingPush == 120 );

}

/*********************************************************************************************************************/
/* test case for four scalar accessors in push mode: one feeder and three consumers */

BOOST_AUTO_TEST_CASE_TEMPLATE( testFourScalarPushAccessors, T, test_types ) {
  std::cout << "*** testFourScalarPushAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingPush >> app.testModule.consumingPush;
  app.testModule.feedingPush >> app.testModule.consumingPush2;
  app.testModule.feedingPush >> app.testModule.consumingPush3;
  app.initialise();

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
  BOOST_CHECK(app.testModule.consumingPush2 ==42);
  BOOST_CHECK(app.testModule.consumingPush3 == 42);

  // launch read() on the consumers asynchronously and make sure it does not yet receive anything
  auto futRead = std::async(std::launch::async, [&app]{ app.testModule.consumingPush.read(); });
  auto futRead2 = std::async(std::launch::async, [&app]{ app.testModule.consumingPush2.read(); });
  auto futRead3 = std::async(std::launch::async, [&app]{ app.testModule.consumingPush3.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout);
  BOOST_CHECK(futRead3.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout);

  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 ==42);
  BOOST_CHECK(app.testModule.consumingPush3 == 42);

  // write to the feeder
  app.testModule.feedingPush = 120;
  app.testModule.feedingPush.write();

  // check that the consumers now receive the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(futRead2.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK(futRead3.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  BOOST_CHECK( app.testModule.consumingPush == 120 );
  BOOST_CHECK( app.testModule.consumingPush2 == 120 );
  BOOST_CHECK( app.testModule.consumingPush3 == 120 );

}

/*********************************************************************************************************************/
/* test case for two scalar accessors, feeder in push mode and consumer in poll mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoScalarPushPollAccessors, T, test_types ) {
  std::cout << "*** testTwoScalarPushPollAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.feedingPush >> app.testModule.consumingPoll;
  app.initialise();

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
  BOOST_CHECK( app.testModule.consumingPoll == 120 );
  app.testModule.consumingPoll.read();
  BOOST_CHECK( app.testModule.consumingPoll == 120 );
  app.testModule.consumingPoll.read();
  BOOST_CHECK( app.testModule.consumingPoll == 120 );

}

/*********************************************************************************************************************/
/* test case for two array accessors in push mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoArrayAccessors, T, test_types ) {
  std::cout << "*** testTwoArrayAccessors<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;
  
  app.testModule.feedingArray >> app.testModule.consumingPushArray;
  app.initialise();
  
  BOOST_CHECK(app.testModule.feedingArray.getNumberOfElements() == 10);
  BOOST_CHECK(app.testModule.consumingPushArray.getNumberOfElements() == 10);
  BOOST_CHECK(app.testModule.feedingArray.getNElements() == 10);
  BOOST_CHECK(app.testModule.consumingPushArray.getNElements() == 10);

  // single theaded test
  for(auto &val : app.testModule.consumingPushArray) val = 0;
  for(unsigned int i=0; i<10; ++i) app.testModule.feedingArray[i] = 99+(T)i;
  for(auto &val : app.testModule.consumingPushArray) BOOST_CHECK(val == 0);
  app.testModule.feedingArray.write();
  for(auto &val : app.testModule.consumingPushArray) BOOST_CHECK(val == 0);
  app.testModule.consumingPushArray.read();
  for(unsigned int i=0; i<10; ++i) BOOST_CHECK(app.testModule.consumingPushArray[i] == 99+(T)i);

  // launch read() on the consumer asynchronously and make sure it does not yet receive anything
  auto futRead = std::async(std::launch::async, [&app]{ app.testModule.consumingPushArray.read(); });
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout);

  for(unsigned int i=0; i<10; ++i) BOOST_CHECK(app.testModule.consumingPushArray[i] == 99+(T)i);

  // write to the feeder
  for(unsigned int i=0; i<10; ++i) app.testModule.feedingArray[i] = 42-(T)i;
  app.testModule.feedingArray.write();

  // check that the consumer now receives the just written value
  BOOST_CHECK(futRead.wait_for(std::chrono::milliseconds(2000)) == std::future_status::ready);
  for(unsigned int i=0; i<10; ++i) BOOST_CHECK(app.testModule.consumingPushArray[i] == 42-(T)i);

}
