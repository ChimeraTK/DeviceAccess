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
#include "VariableGroup.h"
#include "TestFacility.h"

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
        usleep(10000);  // wait some extra time to make sure we are really blocking the test procedure thread
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
        usleep(10000);  // wait some extra time to make sure we are really blocking the test procedure thread
        someOutput.write();
      }
    }
};

/*********************************************************************************************************************/
/* the ReadAnyTestModule calls readAny on a bunch of inputs and outputs some information on the received data */

template<typename T>
struct ReadAnyTestModule : public ctk::ApplicationModule {
    ReadAnyTestModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    struct Inputs : public ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<T> v1{this, "v1", "cm", "Input 1 for testing"};
      ctk::ScalarPushInput<T> v2{this, "v2", "cm", "Input 2 for testing"};
      ctk::ScalarPushInput<T> v3{this, "v3", "cm", "Input 3 for testing"};
      ctk::ScalarPushInput<T> v4{this, "v4", "cm", "Input 4 for testing"};
    };
    Inputs inputs{this, "inputs"};
    ctk::ScalarOutput<T> value{this, "value", "cm", "The last value received from any of the inputs"};
    ctk::ScalarOutput<uint32_t> index{this, "index", "", "The index (1..4) of the input where the last value was received"};

    void mainLoop() {
      while(true) {
        auto justRead = inputs.readAny();
        if(inputs.v1.isSameRegister(justRead)) {
          index = 1;
          value = (T)inputs.v1;
        }
        else if(inputs.v2.isSameRegister(justRead)) {
          index = 2;
          value = (T)inputs.v2;
        }
        else if(inputs.v3.isSameRegister(justRead)) {
          index = 3;
          value = (T)inputs.v3;
        }
        else if(inputs.v4.isSameRegister(justRead)) {
          index = 4;
          value = (T)inputs.v4;
        }
        else {
          index = 0;
          value = 0;
        }
        usleep(10000);  // wait some extra time to make sure we are really blocking the test procedure thread
        index.write();
        value.write();
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

    ctk::ControlSystemModule cs{""};
    BlockingReadTestModule<T> blockingReadTestModule{this,"blockingReadTestModule"};
    AsyncReadTestModule<T> asyncReadTestModule{this,"asyncReadTestModule"};
    ReadAnyTestModule<T> readAnyTestModule{this,"readAnyTestModule"};
};

/*********************************************************************************************************************/
/* test that no TestDecoratorRegisterAccessor is used if the testable mode is not enabled */

BOOST_AUTO_TEST_CASE_TEMPLATE( testNoDecorator, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testNoDecorator<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);
  
  app.blockingReadTestModule >= app.cs["blocking"];
  app.asyncReadTestModule >= app.cs["async"];
  app.readAnyTestModule >= app.cs["readAny"];

  app.initialise();
  app.run();
  
  // check if we got the decorator for the input
  auto hlinput = app.blockingReadTestModule.someInput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hlinput) == nullptr );

  // check that we did not get the decorator for the output
  auto hloutput = app.blockingReadTestModule.someOutput.getHighLevelImplElement();
  BOOST_CHECK( boost::dynamic_pointer_cast<ctk::TestDecoratorRegisterAccessor<T>>(hloutput) == nullptr );

}

/*********************************************************************************************************************/
/* test blocking read in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testBlockingRead, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testBlockingRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.cs("input") >> app.blockingReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.cs("output");
  app.asyncReadTestModule >= app.cs["async"]; // avoid runtime warning
  app.readAnyTestModule >= app.cs["readAny"]; // avoid runtime warning
  
  ctk::TestFacility test;
  auto pvInput = test.getScalar<T>("input");
  auto pvOutput = test.getScalar<T>("output");
  test.runApplication();

  // test blocking read when taking control in the test thread (note: the blocking read is executed in the app module!)
  for(int i=0; i<5; ++i) {
    pvInput = 120+i;
    pvInput.write();
    usleep(10000);
    BOOST_CHECK(pvOutput.readNonBlocking() == false);
    test.stepApplication();
    CHECK_TIMEOUT(pvOutput.readNonBlocking() == true, 200);
    int val = pvOutput;
    BOOST_CHECK(val == 120+i);
  }

}

/*********************************************************************************************************************/
/* test async read in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testAsyncRead, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testAsyncRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.cs("input") >> app.asyncReadTestModule.someInput;
  app.asyncReadTestModule.someOutput >> app.cs("output");
  app.blockingReadTestModule >= app.cs["blocking"]; // avoid runtime warning
  app.readAnyTestModule >= app.cs["readAny"]; // avoid runtime warning

  ctk::TestFacility test;
  
  auto pvInput = test.getScalar<T>("input");
  auto pvOutput = test.getScalar<T>("output");
  
  test.runApplication();

  // test blocking read when taking control in the test thread
  for(int i=0; i<5; ++i) {
    pvInput = 120+i;
    pvInput.write();
    usleep(10000);
    BOOST_CHECK(pvOutput.readNonBlocking() == false);
    test.stepApplication();
    bool ret = pvOutput.readNonBlocking();
    BOOST_CHECK(ret == true);
    if(!ret) {
      CHECK_TIMEOUT(pvOutput.readNonBlocking() == true, 10000);
    }
    int val = pvOutput;
    BOOST_CHECK(val == 120+i);
  }

}

/*********************************************************************************************************************/
/* test testReadAny in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testReadAny, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testReadAny<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.readAnyTestModule.inputs >= app.cs["input"];
  app.readAnyTestModule.value >> app.cs("value");
  app.readAnyTestModule.index >> app.cs("index");
  app.blockingReadTestModule >= app.cs["blocking"];  // avoid runtime warning
  app.asyncReadTestModule >= app.cs["async"];  // avoid runtime warning

  ctk::TestFacility test;
  auto value = test.getScalar<T>("value");
  auto index = test.getScalar<uint32_t>("index");
  auto v1 = test.getScalar<T>("input/v1");
  auto v2 = test.getScalar<T>("input/v2");
  auto v3 = test.getScalar<T>("input/v3");
  auto v4 = test.getScalar<T>("input/v4");
  test.runApplication();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // send something to v4
  v4 = 66;
  v4.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 66);
  BOOST_CHECK(index == 4);
  
  // send something to v1
  v1 = 33;
  v1.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 33);
  BOOST_CHECK(index == 1);
  
  // send something to v1 again
  v1 = 34;
  v1.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  test.stepApplication();

  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 34);
  BOOST_CHECK(index == 1);
  
  // send something to v3
  v3 = 40;
  v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 40);
  BOOST_CHECK(index == 3);
  
  // send something to v2
  v2 = 50;
  v2.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 50);
  BOOST_CHECK(index == 2);

  // check that stepApplication() throws an exception if no input data is available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>) {
  }

  // check that we still don't receive anything anymore
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // send something to v1 a 3rd time
  v1 = 35;
  v1.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 35);
  BOOST_CHECK(index == 1);
  
}

/*********************************************************************************************************************/
/* test the interplay of multiple chained modules and their threads in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testChainedModules, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testChainedModules<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  // put everything we got into one chain
  app.readAnyTestModule.inputs >= app.cs["input"];
  app.readAnyTestModule.value >> app.blockingReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.asyncReadTestModule.someInput;
  app.asyncReadTestModule.someOutput >> app.cs("value");
  app.readAnyTestModule.index >> app.cs("index");

  ctk::TestFacility test;
  auto value = test.getScalar<T>("value");
  auto index = test.getScalar<uint32_t>("index");
  auto v1 = test.getScalar<T>("input/v1");
  auto v2 = test.getScalar<T>("input/v2");
  auto v3 = test.getScalar<T>("input/v3");
  auto v4 = test.getScalar<T>("input/v4");
  test.runApplication();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // send something to v2
  v2 = 11;
  v2.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 11);
  BOOST_CHECK(index == 2);
  
  // send something to v3
  v3 = 12;
  v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 12);
  BOOST_CHECK(index == 3);
  
  // send something to v3 again
  v3 = 13;
  v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 13);
  BOOST_CHECK(index == 3);

  // check that stepApplication() throws an exception if no input data is available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>) {
  }

  // check that we still don't receive anything anymore
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

}
