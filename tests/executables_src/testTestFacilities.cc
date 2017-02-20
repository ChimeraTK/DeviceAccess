/*
 * testTestFacilities.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: Martin Hierholzer
 */

#include <future>
#include <chrono>

#define BOOST_TEST_MODULE testTestFacilities

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ControlSystemModule.h"
#include "TestDecoratorRegisterAccessor.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                   \
    {                                                                                                               \
      std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                  \
      while(!(condition)) {                                                                                         \
        bool timeout_reached = (std::chrono::steady_clock::now()-t0) > std::chrono::milliseconds(maxMilliseconds);  \
        BOOST_CHECK( !timeout_reached );                                                                            \
        if(timeout_reached) break;                                                                                  \
        usleep(1000);                                                                                               \
      }                                                                                                             \
    }

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

    ctk::ScalarPushInput<T> someInput{this, "someInput", "cm", "Description"};
    ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
    TestApplication() : Application("test application") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests

    TestModule<T> testModule{this,"testModule"};
};

/*********************************************************************************************************************/
/* test that no TestDecoratorRegisterAccessor is used if the testable mode is not enabled */

BOOST_AUTO_TEST_CASE_TEMPLATE( testNoDecorator, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testNoDecorator<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.someOutput >> app.testModule.someInput;
  app.initialise();
  app.run();
  
  // check if we got the decorator for the input
  auto hlinput = app.testModule.someInput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hlinput) == nullptr );

  // check that we did not get the decorator for the output
  auto hloutput = app.testModule.someOutput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hloutput) == nullptr );

}

/*********************************************************************************************************************/
/* simply test the TestDecoratorRegisterAccessor if it is used and if it properly works as a decorator */

BOOST_AUTO_TEST_CASE_TEMPLATE( testDecorator, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testDecorator<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.testModule.someOutput >> app.testModule.someInput;
  app.enableTestableMode();
  app.initialise();
  app.run();
  
  // check if we got the decorator for the input
  auto hlinput = app.testModule.someInput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hlinput) != nullptr );

  // check that we did not get the decorator for the output
  auto hloutput = app.testModule.someOutput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hloutput) == nullptr );

}
