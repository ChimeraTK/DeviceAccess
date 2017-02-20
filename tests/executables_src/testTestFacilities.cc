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

#include <mtca4u/ExperimentalFeatures.h>

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
/* the NoLoopTestModule is a dummy test module with an empty mainLoop */

template<typename T>
struct NoLoopTestModule : public ctk::ApplicationModule {
    NoLoopTestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    ctk::ScalarPushInput<T> someInput{this, "someInput", "cm", "This is just some input for testing"};
    ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

    void mainLoop() {
    }
};

/*********************************************************************************************************************/
/* the BlockingReadTestModule blockingly reads its input in the main loop and writes the result to its output */

template<typename T>
struct BlockingReadTestModule : public ctk::ApplicationModule {
    BlockingReadTestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    ctk::ScalarPushInput<T> someInput{this, "someInput", "cm", "This is just some input for testing"};
    ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

    void mainLoop() {
      while(true) {
        someInput.read();
        T val = someInput;
        someOutput = val;
        someOutput.write();
      }
    }
};

/*********************************************************************************************************************/
/* the AsyncReadTestModule asynchronously reads its input in the main loop and writes the result to its output */

template<typename T>
struct AsyncReadTestModule : public ctk::ApplicationModule {
    AsyncReadTestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    ctk::ScalarPushInput<T> someInput{this, "someInput", "cm", "This is just some input for testing"};
    ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

    void mainLoop() {
      while(true) {
        auto &future = someInput.readAsync();
        future.wait();
        T val = someInput;
        someOutput = val;
        someOutput.write();
      }
    }
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
    TestApplication() : Application("test application") {
      ChimeraTK::ExperimentalFeatures::enable();
    }
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests

    NoLoopTestModule<T> noLoopTestModule{this,"noLoopTestModule"};
    BlockingReadTestModule<T> blockingReadTestModule{this,"blockingReadTestModule"};
    AsyncReadTestModule<T> asyncReadTestModule{this,"asyncReadTestModule"};
};

/*********************************************************************************************************************/
/* test that no TestDecoratorRegisterAccessor is used if the testable mode is not enabled */

BOOST_AUTO_TEST_CASE_TEMPLATE( testNoDecorator, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testNoDecorator<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.noLoopTestModule.someOutput >> app.noLoopTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.blockingReadTestModule.someInput; // just to avoid runtime warning
  app.asyncReadTestModule.someOutput >> app.asyncReadTestModule.someInput; // just to avoid runtime warning
  app.initialise();
  app.run();
  
  // check if we got the decorator for the input
  auto hlinput = app.noLoopTestModule.someInput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hlinput) == nullptr );

  // check that we did not get the decorator for the output
  auto hloutput = app.noLoopTestModule.someOutput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hloutput) == nullptr );

}

/*********************************************************************************************************************/
/* simply test the TestDecoratorRegisterAccessor if it is used and if it properly works as a decorator */

BOOST_AUTO_TEST_CASE_TEMPLATE( testDecorator, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testDecorator<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.noLoopTestModule.someOutput >> app.noLoopTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.blockingReadTestModule.someInput; // just to avoid runtime warning
  app.asyncReadTestModule.someOutput >> app.asyncReadTestModule.someInput; // just to avoid runtime warning
  app.enableTestableMode();
  app.initialise();
  app.run();
  app.resumeApplication();  // don't take control in this test

  // check if we got the decorator for the input
  auto hlinput = app.noLoopTestModule.someInput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hlinput) != nullptr );

  // check that we did not get the decorator for the output
  auto hloutput = app.noLoopTestModule.someOutput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hloutput) == nullptr );
  
  // check meta-data
  BOOST_CHECK(app.noLoopTestModule.someInput.isReadOnly());
  BOOST_CHECK(app.noLoopTestModule.someInput.isReadable());
  BOOST_CHECK(!app.noLoopTestModule.someInput.isWriteable());

  // test non blocking read
  app.noLoopTestModule.someInput = 41;
  for(int i=0; i<5; ++i) {
    app.noLoopTestModule.someOutput = 42+i;
    bool ret = app.noLoopTestModule.someInput.readNonBlocking();
    BOOST_CHECK(ret == false);
    int val = app.noLoopTestModule.someInput;
    BOOST_CHECK(val == 42+i-1);
    app.noLoopTestModule.someOutput.write();
    ret = app.noLoopTestModule.someInput.readNonBlocking();
    BOOST_CHECK(ret == true);
    val = app.noLoopTestModule.someInput;
    BOOST_CHECK(val == 42+i);
  }
  bool ret = app.noLoopTestModule.someInput.readNonBlocking();
  BOOST_CHECK(ret == false);

  // test blocking read
  for(int i=0; i<5; ++i) {
    auto future = std::async(std::launch::async, [&app] { app.noLoopTestModule.someInput.read(); });
    app.noLoopTestModule.someOutput = 120+i;
    BOOST_CHECK(future.wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout);
    app.noLoopTestModule.someOutput.write();
    BOOST_CHECK(future.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready);
    int val = app.noLoopTestModule.someInput;
    BOOST_CHECK(val == 120+i);
  }
  
}

/*********************************************************************************************************************/
/* test blocking read in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testBlockingRead, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testBlockingRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.noLoopTestModule.someOutput >> app.blockingReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.noLoopTestModule.someInput;
  app.asyncReadTestModule.someOutput >> app.asyncReadTestModule.someInput; // just to avoid runtime warning
  app.enableTestableMode();
  app.initialise();
  app.run();

  // test blocking read when taking control in the test thread
  for(int i=0; i<5; ++i) {
    app.noLoopTestModule.someOutput = 120+i;
    app.noLoopTestModule.someOutput.write();
    usleep(10000);
    BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
    app.stepApplication();
    CHECK_TIMEOUT(app.noLoopTestModule.someInput.readNonBlocking() == true, 200);
    int val = app.noLoopTestModule.someInput;
    BOOST_CHECK(val == 120+i);
  }

}

/*********************************************************************************************************************/
/* test async read in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testAsyncRead, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testAsyncRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.noLoopTestModule.someOutput >> app.asyncReadTestModule.someInput;
  app.asyncReadTestModule.someOutput >> app.noLoopTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.blockingReadTestModule.someInput; // just to avoid runtime warning
  app.enableTestableMode();
  app.initialise();
  app.run();

  // test blocking read when taking control in the test thread
  for(int i=0; i<5; ++i) {
    app.noLoopTestModule.someOutput = 120+i;
    app.noLoopTestModule.someOutput.write();
    usleep(10000);
    BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
    app.stepApplication();
    CHECK_TIMEOUT(app.noLoopTestModule.someInput.readNonBlocking() == true, 200);
    int val = app.noLoopTestModule.someInput;
    BOOST_CHECK(val == 120+i);
  }

}
