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
    ctk::ScalarPushInput<uint32_t> someUIntInput{this, "someUIntInput", "", "Unsigned integer"};
    ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

    struct Outputs : public ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarOutput<T> v1{this, "v1", "cm", "Output 1 for testing"};
      ctk::ScalarOutput<T> v2{this, "v2", "cm", "Output 2 for testing"};
      ctk::ScalarOutput<T> v3{this, "v3", "cm", "Output 3 for testing"};
      ctk::ScalarOutput<T> v4{this, "v4", "cm", "Output 4 for testing"};
    };
    Outputs outputs{this, "outputs"};

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

    NoLoopTestModule<T> noLoopTestModule{this,"noLoopTestModule"};
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

  app.noLoopTestModule.someOutput >> app.noLoopTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.blockingReadTestModule.someInput; // just to avoid runtime warning
  app.asyncReadTestModule.someOutput >> app.asyncReadTestModule.someInput; // just to avoid runtime warning
  app.noLoopTestModule.outputs >= app.readAnyTestModule.inputs;  // just to avoid runtime warning
  app.readAnyTestModule.value >> ctk::VariableNetworkNode::makeConstant<T>(false, 0, 1); // just to avoid runtime warning
  app.readAnyTestModule.index >> app.noLoopTestModule.someUIntInput; // just to avoid runtime warning
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
  app.noLoopTestModule.outputs >= app.readAnyTestModule.inputs;  // just to avoid runtime warning
  app.readAnyTestModule.value >> ctk::VariableNetworkNode::makeConstant<T>(false, 0, 1); // just to avoid runtime warning
  app.readAnyTestModule.index >> app.noLoopTestModule.someUIntInput; // just to avoid runtime warning
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
  app.noLoopTestModule.outputs >= app.readAnyTestModule.inputs;  // just to avoid runtime warning
  app.readAnyTestModule.value >> ctk::VariableNetworkNode::makeConstant<T>(false, 0, 1); // just to avoid runtime warning
  app.readAnyTestModule.index >> app.noLoopTestModule.someUIntInput; // just to avoid runtime warning
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
  app.noLoopTestModule.outputs >= app.readAnyTestModule.inputs;  // just to avoid runtime warning
  app.readAnyTestModule.value >> ctk::VariableNetworkNode::makeConstant<T>(false, 0, 1); // just to avoid runtime warning
  app.readAnyTestModule.index >> app.noLoopTestModule.someUIntInput; // just to avoid runtime warning
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
/* test testReadAny in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testReadAny, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testReadAny<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.noLoopTestModule.someOutput >> app.asyncReadTestModule.someInput; // just to avoid runtime warning
  app.asyncReadTestModule.someOutput >> ctk::VariableNetworkNode::makeConstant<T>(false, 0, 1); // just to avoid runtime warning
  app.blockingReadTestModule.someOutput >> app.blockingReadTestModule.someInput; // just to avoid runtime warning
  app.noLoopTestModule.outputs >= app.readAnyTestModule.inputs;
  app.readAnyTestModule.value >> app.noLoopTestModule.someInput;
  app.readAnyTestModule.index >> app.noLoopTestModule.someUIntInput;
  app.enableTestableMode();
  app.initialise();
  app.run();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // send something to v4
  app.noLoopTestModule.outputs.v4 = 66;
  app.noLoopTestModule.outputs.v4.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 66);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 4);
  
  // send something to v1
  app.noLoopTestModule.outputs.v1 = 33;
  app.noLoopTestModule.outputs.v1.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 33);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 1);
  
  // send something to v1 again
  app.noLoopTestModule.outputs.v1 = 34;
  app.noLoopTestModule.outputs.v1.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 34);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 1);
  
  // send something to v3
  app.noLoopTestModule.outputs.v3 = 40;
  app.noLoopTestModule.outputs.v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 40);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 3);
  
  // send something to v2
  app.noLoopTestModule.outputs.v2 = 50;
  app.noLoopTestModule.outputs.v2.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 50);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 2);
  
  // send something to v1 a 3rd time
  app.noLoopTestModule.outputs.v1 = 35;
  app.noLoopTestModule.outputs.v1.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 35);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 1);

  // check that we still don't receive anything anymore, even if we try running the application again
  app.stepApplication();
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
}

/*********************************************************************************************************************/
/* test the interplay of multiple chained modules and their threads in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE( testChainedModules, T, test_types ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testChainedModules<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  // put everything we got into one chain
  app.noLoopTestModule.outputs >= app.readAnyTestModule.inputs;
  app.readAnyTestModule.value >> app.blockingReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.asyncReadTestModule.someInput;
  app.asyncReadTestModule.someOutput >> app.noLoopTestModule.someInput;
  app.readAnyTestModule.index >> app.noLoopTestModule.someUIntInput;
  app.noLoopTestModule.someOutput >> ctk::VariableNetworkNode::makeConstant<T>(false, 0, 1); // just to avoid runtime warning

  app.enableTestableMode();
  app.initialise();
  app.run();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // send something to v2
  app.noLoopTestModule.outputs.v2 = 11;
  app.noLoopTestModule.outputs.v2.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 11);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 2);
  
  // send something to v3
  app.noLoopTestModule.outputs.v3 = 12;
  app.noLoopTestModule.outputs.v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 12);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 3);
  
  // send something to v3 again
  app.noLoopTestModule.outputs.v3 = 13;
  app.noLoopTestModule.outputs.v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);
  
  // run the application and check that we got the expected result
  app.stepApplication();
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == true);
  BOOST_CHECK(app.noLoopTestModule.someInput == 13);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput == 3);

  // check that we still don't receive anything anymore, even if we try running the application again
  app.stepApplication();
  usleep(10000);
  BOOST_CHECK(app.noLoopTestModule.someInput.readNonBlocking() == false);
  BOOST_CHECK(app.noLoopTestModule.someUIntInput.readNonBlocking() == false);

}
