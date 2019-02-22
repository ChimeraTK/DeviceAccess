/*
 * testTestFacilities.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: Martin Hierholzer
 */

#include <chrono>
#include <future>

#define BOOST_TEST_MODULE testTestFacilities

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <ChimeraTK/Device.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "TestableModeAccessorDecorator.h"
#include "VariableGroup.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                      \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(!(condition)) {                                                                                              \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) break;                                                                                       \
      usleep(1000);                                                                                                    \
    }                                                                                                                  \
  }

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, float, double> test_types;

constexpr char dummySdm[] = "sdm://./dummy=test.map";

/*********************************************************************************************************************/
/* the BlockingReadTestModule blockingly reads its input in the main loop and
 * writes the result to its output */

template<typename T>
struct BlockingReadTestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<T> someInput{this, "someInput", "cm", "This is just some input for testing"};
  ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

  void mainLoop() {
    while(true) {
      someInput.read();
      T val = someInput;
      someOutput = val;
      usleep(10000); // wait some extra time to make sure we are really blocking
                     // the test procedure thread
      someOutput.write();
    }
  }
};

/*********************************************************************************************************************/
/* the AsyncReadTestModule asynchronously reads its input in the main loop and
 * writes the result to its output */

template<typename T>
struct AsyncReadTestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<T> someInput{this, "someInput", "cm", "This is just some input for testing"};
  ctk::ScalarOutput<T> someOutput{this, "someOutput", "cm", "Description"};

  void mainLoop() {
    while(true) {
      auto future = someInput.readAsync();
      future.wait();
      T val = someInput;
      someOutput = val;
      usleep(10000); // wait some extra time to make sure we are really blocking
                     // the test procedure thread
      someOutput.write();
    }
  }
};

/*********************************************************************************************************************/
/* the ReadAnyTestModule calls readAny on a bunch of inputs and outputs some
 * information on the received data */

template<typename T>
struct ReadAnyTestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  struct Inputs : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<T> v1{this, "v1", "cm", "Input 1 for testing"};
    ctk::ScalarPushInput<T> v2{this, "v2", "cm", "Input 2 for testing"};
    ctk::ScalarPushInput<T> v3{this, "v3", "cm", "Input 3 for testing"};
    ctk::ScalarPushInput<T> v4{this, "v4", "cm", "Input 4 for testing"};
  };
  Inputs inputs{this, "inputs", "A group of inputs"};
  ctk::ScalarOutput<T> value{this, "value", "cm", "The last value received from any of the inputs"};
  ctk::ScalarOutput<uint32_t> index{
      this, "index", "", "The index (1..4) of the input where the last value was received"};

  void mainLoop() {
    auto group = inputs.readAnyGroup();
    while(true) {
      auto justRead = group.readAny();
      if(inputs.v1.getId() == justRead) {
        index = 1;
        value = (T)inputs.v1;
      }
      else if(inputs.v2.getId() == justRead) {
        index = 2;
        value = (T)inputs.v2;
      }
      else if(inputs.v3.getId() == justRead) {
        index = 3;
        value = (T)inputs.v3;
      }
      else if(inputs.v4.getId() == justRead) {
        index = 4;
        value = (T)inputs.v4;
      }
      else {
        index = 0;
        value = 0;
      }
      usleep(10000); // wait some extra time to make sure we are really blocking
                     // the test procedure thread
      index.write();
      value.write();
    }
  }
};

/*********************************************************************************************************************/
/* the PollingReadModule is designed to test poll-type transfers (even mixed
 * with push-type) */

template<typename T>
struct PollingReadModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<T> push{this, "push", "cm", "A push-type input"};
  ctk::ScalarPushInput<T> push2{this, "push2", "cm", "A second push-type input"};
  ctk::ScalarPollInput<T> poll{this, "poll", "cm", "A poll-type input"};

  ctk::ScalarOutput<T> valuePush{this, "valuePush", "cm", "The last value received for 'push'"};
  ctk::ScalarOutput<T> valuePoll{this, "valuePoll", "cm", "The last value received for 'poll'"};
  ctk::ScalarOutput<int> state{this, "state", "", "State of the test mainLoop"};

  void mainLoop() {
    while(true) {
      push.read();
      poll.read();
      valuePush = (T)push;
      valuePoll = (T)poll;
      valuePoll.write();
      valuePush.write();
      state = 1;
      state.write();

      push2.read();
      push.readNonBlocking();
      poll.read();
      valuePush = (T)push;
      valuePoll = (T)poll;
      valuePoll.write();
      valuePush.write();
      state = 2;
      state.write();

      push2.read();
      push.readLatest();
      poll.read();
      valuePush = (T)push;
      valuePoll = (T)poll;
      valuePoll.write();
      valuePush.write();
      state = 3;
      state.write();
    }
  }
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testApplication") {}
  ~TestApplication() { shutdown(); }

  using Application::makeConnections; // we call makeConnections() manually in
                                      // the tests to catch exceptions etc.
  void defineConnections() {}         // setup is done in the tests

  ctk::ControlSystemModule cs{""};
  ctk::DeviceModule dev{dummySdm, ""};
  BlockingReadTestModule<T> blockingReadTestModule{this, "blockingReadTestModule", "Module for testing blocking read"};
  AsyncReadTestModule<T> asyncReadTestModule{this, "asyncReadTestModule", "Module for testing async read"};
  ReadAnyTestModule<T> readAnyTestModule{this, "readAnyTestModule", "Module for testing readAny()"};
};

/*********************************************************************************************************************/
/* second application */

template<typename T>
struct PollingTestApplication : public ctk::Application {
  PollingTestApplication() : Application("testApplication") {}
  ~PollingTestApplication() { shutdown(); }

  void defineConnections() {} // setup is done in the tests

  ctk::ControlSystemModule cs{""};
  PollingReadModule<T> pollingReadModule{this, "pollingReadModule", "Module for testing poll-type transfers"};
};

/*********************************************************************************************************************/
/* test that no TestableModeAccessorDecorator is used if the testable mode is
 * not enabled */

BOOST_AUTO_TEST_CASE_TEMPLATE(testNoDecorator, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testNoDecorator<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  auto pvManagers = ctk::createPVManager();
  app.setPVManager(pvManagers.second);

  app.blockingReadTestModule.connectTo(app.cs["blocking"]);
  app.asyncReadTestModule.connectTo(app.cs["async"]);
  app.readAnyTestModule.connectTo(app.cs["readAny"]);

  app.initialise();
  app.run();

  // check if we got the decorator for the input
  auto hlinput = app.blockingReadTestModule.someInput.getHighLevelImplElement();
  BOOST_CHECK(boost::dynamic_pointer_cast<ctk::TestableModeAccessorDecorator<T>>(hlinput) == nullptr);

  // check that we did not get the decorator for the output
  auto hloutput = app.blockingReadTestModule.someOutput.getHighLevelImplElement();
  BOOST_CHECK(boost::dynamic_pointer_cast<ctk::TestableModeAccessorDecorator<T>>(hloutput) == nullptr);
}

/*********************************************************************************************************************/
/* test blocking read in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE(testBlockingRead, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testBlockingRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.cs("input") >> app.blockingReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.cs("output");
  app.asyncReadTestModule.connectTo(app.cs["async"]); // avoid runtime warning
  app.readAnyTestModule.connectTo(app.cs["readAny"]); // avoid runtime warning

  ctk::TestFacility test;
  auto pvInput = test.getScalar<T>("input");
  auto pvOutput = test.getScalar<T>("output");
  test.runApplication();

  // test blocking read when taking control in the test thread (note: the
  // blocking read is executed in the app module!)
  for(int i = 0; i < 5; ++i) {
    pvInput = 120 + i;
    pvInput.write();
    usleep(10000);
    BOOST_CHECK(pvOutput.readNonBlocking() == false);
    test.stepApplication();
    CHECK_TIMEOUT(pvOutput.readNonBlocking() == true, 200);
    int val = pvOutput;
    BOOST_CHECK(val == 120 + i);
  }
}

/*********************************************************************************************************************/
/* test async read in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE(testAsyncRead, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testAsyncRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.cs("input") >> app.asyncReadTestModule.someInput;
  app.asyncReadTestModule.someOutput >> app.cs("output");
  app.blockingReadTestModule.connectTo(app.cs["blocking"]); // avoid runtime warning
  app.readAnyTestModule.connectTo(app.cs["readAny"]);       // avoid runtime warning

  ctk::TestFacility test;

  auto pvInput = test.getScalar<T>("input");
  auto pvOutput = test.getScalar<T>("output");

  test.runApplication();

  // test blocking read when taking control in the test thread
  for(int i = 0; i < 5; ++i) {
    pvInput = 120 + i;
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
    BOOST_CHECK(val == 120 + i);
  }
}

/*********************************************************************************************************************/
/* test testReadAny in test mode */

BOOST_AUTO_TEST_CASE_TEMPLATE(testReadAny, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testReadAny<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.readAnyTestModule.inputs.connectTo(app.cs["input"]);
  app.readAnyTestModule.value >> app.cs("value");
  app.readAnyTestModule.index >> app.cs("index");
  app.blockingReadTestModule.connectTo(app.cs["blocking"]); // avoid runtime warning
  app.asyncReadTestModule.connectTo(app.cs["async"]);       // avoid runtime warning

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

  // check that stepApplication() throws an exception if no input data is
  // available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
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
/* test the interplay of multiple chained modules and their threads in test mode
 */

BOOST_AUTO_TEST_CASE_TEMPLATE(testChainedModules, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testChainedModules<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  // put everything we got into one chain
  app.readAnyTestModule.inputs.connectTo(app.cs["input"]);
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
  test.stepApplication();
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
  test.stepApplication();
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
  test.stepApplication();
  BOOST_CHECK(value.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(value == 13);
  BOOST_CHECK(index == 3);

  // check that stepApplication() throws an exception if no input data is
  // available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }

  // check that we still don't receive anything anymore
  usleep(10000);
  BOOST_CHECK(value.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
}

/*********************************************************************************************************************/
/* test combination with fan out */

BOOST_AUTO_TEST_CASE_TEMPLATE(testWithFanOut, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testWithFanOut<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  // distribute a value to multiple inputs
  app.readAnyTestModule.inputs.connectTo(app.cs["input"]);
  app.readAnyTestModule.value >> app.blockingReadTestModule.someInput >> app.asyncReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.cs("valueFromBlocking");
  app.asyncReadTestModule.someOutput >> app.cs("valueFromAsync");
  app.readAnyTestModule.index >> app.cs("index");

  ctk::TestFacility test;
  auto valueFromBlocking = test.getScalar<T>("valueFromBlocking");
  auto valueFromAsync = test.getScalar<T>("valueFromAsync");
  auto index = test.getScalar<uint32_t>("index");
  auto v1 = test.getScalar<T>("input/v1");
  auto v2 = test.getScalar<T>("input/v2");
  auto v3 = test.getScalar<T>("input/v3");
  auto v4 = test.getScalar<T>("input/v4");
  test.runApplication();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // send something to v2
  v2 = 11;
  v2.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK_EQUAL((T)valueFromBlocking, 11);
  BOOST_CHECK_EQUAL((T)valueFromAsync, 11);
  BOOST_CHECK_EQUAL((unsigned int)index, 2);

  // send something to v3
  v3 = 12;
  v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK_EQUAL((T)valueFromBlocking, 12);
  BOOST_CHECK_EQUAL((T)valueFromAsync, 12);
  BOOST_CHECK_EQUAL((unsigned int)index, 3);

  // send something to v3 again
  v3 = 13;
  v3.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK_EQUAL((T)valueFromBlocking, 13);
  BOOST_CHECK_EQUAL((T)valueFromAsync, 13);
  BOOST_CHECK_EQUAL((unsigned int)index, 3);

  // check that stepApplication() throws an exception if no input data is
  // available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }

  // check that we still don't receive anything anymore
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
}

/*********************************************************************************************************************/
/* test combination with trigger */

BOOST_AUTO_TEST_CASE_TEMPLATE(testWithTrigger, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testWithTrigger<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;
  // distribute a value to multiple inputs
  auto triggernode = app.cs("trigger", typeid(int), 1);
  app.cs("v1") >> app.readAnyTestModule.inputs.v1;
  app.dev("REG2")[triggernode] >> app.readAnyTestModule.inputs.v2;
  app.cs("v3") >> app.readAnyTestModule.inputs.v3;
  app.cs("v4") >> app.readAnyTestModule.inputs.v4;
  app.readAnyTestModule.value >> app.blockingReadTestModule.someInput >> app.asyncReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.cs("valueFromBlocking");
  app.asyncReadTestModule.someOutput >> app.cs("valueFromAsync");
  app.readAnyTestModule.index >> app.cs("index");

  ctk::TestFacility test;
  ctk::Device dev;
  dev.open(dummySdm);
  auto valueFromBlocking = test.getScalar<T>("valueFromBlocking");
  auto valueFromAsync = test.getScalar<T>("valueFromAsync");
  auto index = test.getScalar<uint32_t>("index");
  auto trigger = test.getScalar<int>("trigger");
  auto v2 = dev.getScalarRegisterAccessor<T>("REG2");
  test.runApplication();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // send something to v2 and send the trigger
  v2 = 11;
  v2.write();
  trigger.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(valueFromBlocking == 11);
  BOOST_CHECK(valueFromAsync == 11);
  BOOST_CHECK(index == 2);

  // again send something to v2 and send the trigger
  v2 = 22;
  v2.write();
  trigger.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(valueFromBlocking == 22);
  BOOST_CHECK(valueFromAsync == 22);
  BOOST_CHECK(index == 2);

  // check that stepApplication() throws an exception if no input data is
  // available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }

  // check that we still don't receive anything anymore
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
}

/*********************************************************************************************************************/
/* test combination with trigger distributed to mutliple receivers */

BOOST_AUTO_TEST_CASE_TEMPLATE(testWithTriggerFanOut, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testWithTriggerFanOut<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;
  // distribute a value to multiple inputs
  auto triggernode = app.cs("trigger", typeid(int), 1);
  app.dev("REG1")[triggernode] >> app.readAnyTestModule.inputs.v1;
  app.cs("v2") >> app.readAnyTestModule.inputs.v2;
  app.cs("v3") >> app.readAnyTestModule.inputs.v3;
  app.cs("v4") >> app.readAnyTestModule.inputs.v4;
  app.dev("REG2")[triggernode] >> app.asyncReadTestModule.someInput;
  app.dev("REG3")[triggernode] >> app.blockingReadTestModule.someInput;
  app.readAnyTestModule.value >> app.cs("valueFromAny");
  app.readAnyTestModule.index >> app.cs("index");
  app.blockingReadTestModule.someOutput >> app.cs("valueFromBlocking");
  app.asyncReadTestModule.someOutput >> app.cs("valueFromAsync");

  ctk::TestFacility test;
  ctk::Device dev;
  dev.open(dummySdm);
  auto valueFromBlocking = test.getScalar<T>("valueFromBlocking");
  auto valueFromAsync = test.getScalar<T>("valueFromAsync");
  auto valueFromAny = test.getScalar<T>("valueFromAny");
  auto index = test.getScalar<uint32_t>("index");
  auto trigger = test.getScalar<int>("trigger");
  auto r1 = dev.getScalarRegisterAccessor<T>("REG1");
  auto r2 = dev.getScalarRegisterAccessor<T>("REG2");
  auto r3 = dev.getScalarRegisterAccessor<T>("REG3");
  test.runApplication();

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // send something to the registers and send the trigger
  r1 = 11;
  r2 = 22;
  r3 = 33;
  r1.write();
  r2.write();
  r3.write();
  trigger.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(valueFromAny.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(valueFromAny.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(valueFromBlocking == 33);
  BOOST_CHECK(valueFromAsync == 22);
  BOOST_CHECK(valueFromAny == 11);
  BOOST_CHECK(index == 1);

  // check that we don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // send something else to the registers and send the trigger
  r1 = 6;
  r2 = 5;
  r3 = 4;
  r1.write();
  r2.write();
  r3.write();
  trigger.write();

  // check that we still don't receive anything yet
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(valueFromAny.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);

  // run the application and check that we got the expected result
  test.stepApplication();
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == true);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == true);
  BOOST_CHECK(valueFromAny.readNonBlocking() == true);
  BOOST_CHECK(index.readNonBlocking() == true);
  BOOST_CHECK(valueFromBlocking == 4);
  BOOST_CHECK(valueFromAsync == 5);
  BOOST_CHECK(valueFromAny == 6);
  BOOST_CHECK(index == 1);

  // check that stepApplication() throws an exception if no input data is
  // available
  try {
    test.stepApplication();
    BOOST_ERROR("IllegalParameter exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }

  // check that we still don't receive anything anymore
  usleep(10000);
  BOOST_CHECK(valueFromBlocking.readNonBlocking() == false);
  BOOST_CHECK(valueFromAsync.readNonBlocking() == false);
  BOOST_CHECK(valueFromAny.readNonBlocking() == false);
  BOOST_CHECK(index.readNonBlocking() == false);
}

/*********************************************************************************************************************/
/* test convenience read functions */

BOOST_AUTO_TEST_CASE_TEMPLATE(testConvenienceRead, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testConvenienceRead<" << typeid(T).name() << ">" << std::endl;

  TestApplication<T> app;

  app.cs("input") >> app.blockingReadTestModule.someInput;
  app.blockingReadTestModule.someOutput >> app.cs("output");
  app.asyncReadTestModule.connectTo(app.cs["async"]); // avoid runtime warning
  app.readAnyTestModule.connectTo(app.cs["readAny"]); // avoid runtime warning

  ctk::TestFacility test;
  test.runApplication();

  // test blocking read when taking control in the test thread (note: the
  // blocking read is executed in the app module!)
  for(int i = 0; i < 5; ++i) {
    test.writeScalar<T>("input", 120 + i);
    test.stepApplication();
    CHECK_TIMEOUT(test.readScalar<T>("output") == T(120 + i), 200);
  }

  // same with array function (still a scalar variable behind, but this does not
  // matter)
  for(int i = 0; i < 5; ++i) {
    std::vector<T> myValue{T(120 + i)};
    test.writeArray<T>("input", myValue);
    test.stepApplication();
    CHECK_TIMEOUT(test.readArray<T>("output") == std::vector<T>{T(120 + i)}, 200);
  }
}

/*********************************************************************************************************************/
/* test testable mode when reading from constants */

BOOST_AUTO_TEST_CASE_TEMPLATE(testConstants, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testConstants<" << typeid(T).name() << ">" << std::endl;

  {
    TestApplication<T> app;

    ctk::VariableNetworkNode::makeConstant<T>(true, 18) >> app.blockingReadTestModule.someInput;
    ctk::VariableNetworkNode::makeConstant<T>(true, 20) >> app.asyncReadTestModule.someInput;
    ctk::VariableNetworkNode::makeConstant<T>(true, 22) >> app.readAnyTestModule.inputs.v1;
    ctk::VariableNetworkNode::makeConstant<T>(true, 23) >> app.readAnyTestModule.inputs.v2;
    ctk::VariableNetworkNode::makeConstant<T>(true, 24) >> app.readAnyTestModule.inputs.v3;
    app.blockingReadTestModule.someOutput >> app.cs("blockingOutput");
    app.asyncReadTestModule.someOutput >> app.cs("asyncOutput");
    app.cs("v4") >> app.readAnyTestModule.inputs.v4;
    app.readAnyTestModule.value >> app.cs("value");
    app.readAnyTestModule.index >> app.cs("index");

    ctk::TestFacility test;
    test.runApplication();

    BOOST_CHECK_EQUAL((T)app.blockingReadTestModule.someInput, 18);
    BOOST_CHECK_EQUAL((T)app.asyncReadTestModule.someInput, 20);
    BOOST_CHECK_EQUAL((T)app.readAnyTestModule.inputs.v1, 22);
    BOOST_CHECK_EQUAL((T)app.readAnyTestModule.inputs.v2, 23);
    BOOST_CHECK_EQUAL((T)app.readAnyTestModule.inputs.v3, 24);

    test.writeScalar<T>("v4", 27);
    test.stepApplication();
    BOOST_CHECK_EQUAL(test.readScalar<uint32_t>("index"), 4);
    BOOST_CHECK_EQUAL(test.readScalar<T>("value"), 27);

    test.writeScalar<T>("v4", 30);
    test.stepApplication();
    BOOST_CHECK_EQUAL(test.readScalar<uint32_t>("index"), 4);
    BOOST_CHECK_EQUAL(test.readScalar<T>("value"), 30);
  }

  {
    PollingTestApplication<T> app;

    ctk::VariableNetworkNode::makeConstant<T>(true, 18) >> app.pollingReadModule.push2;
    ctk::VariableNetworkNode::makeConstant<T>(true, 20) >> app.pollingReadModule.poll;
    app.pollingReadModule.connectTo(app.cs);

    ctk::TestFacility test;
    test.runApplication();

    BOOST_CHECK_EQUAL((T)app.pollingReadModule.push2, 18);
    BOOST_CHECK_EQUAL((T)app.pollingReadModule.poll, 20);
    BOOST_CHECK_EQUAL(test.readScalar<T>("push2"), 18);
    BOOST_CHECK_EQUAL(test.readScalar<T>("poll"), 20);

    test.writeScalar<T>("push", 22);
    test.stepApplication();
    BOOST_CHECK_EQUAL(test.readScalar<int>("state"), 1);
    BOOST_CHECK_EQUAL(test.readScalar<T>("valuePush"), 22);
    BOOST_CHECK_EQUAL(test.readScalar<T>("valuePoll"), 20);

    // continuing will now stall the tests
    test.writeScalar<T>("push", 23);
    try {
      test.stepApplication();
      BOOST_ERROR("Exception expected.");
    }
    catch(ctk::Application::TestsStalled&) {
    }
  }
}

/*********************************************************************************************************************/
/* test poll-type transfers mixed with push-type */

BOOST_AUTO_TEST_CASE_TEMPLATE(testPolling, T, test_types) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testPolling<" << typeid(T).name() << ">" << std::endl;

  PollingTestApplication<T> app;
  app.pollingReadModule.connectTo(app.cs);

  ctk::TestFacility test;
  test.runApplication();

  auto pv_push = test.getScalar<T>("push");
  auto pv_push2 = test.getScalar<T>("push2");
  auto pv_poll = test.getScalar<T>("poll");
  auto pv_valuePush = test.getScalar<T>("valuePush");
  auto pv_valuePoll = test.getScalar<T>("valuePoll");
  auto pv_state = test.getScalar<int>("state");

  // write values to 'push' and 'poll' and check result
  pv_push = 120;
  pv_push.write();
  pv_poll = 42;
  pv_poll.write();
  test.stepApplication();
  pv_valuePoll.read();
  pv_valuePush.read();
  pv_state.read();
  BOOST_CHECK_EQUAL((T)pv_valuePoll, 42);
  BOOST_CHECK_EQUAL((T)pv_valuePush, 120);
  BOOST_CHECK_EQUAL((T)pv_state, 1);

  // this time the application gets triggered by push2, push is read
  // non-blockingly (single value only)
  pv_push = 22;
  pv_push.write();
  pv_poll = 44;
  pv_poll.write();
  pv_poll = 45;
  pv_poll.write();
  pv_push2.write();
  test.stepApplication();
  pv_valuePoll.read();
  pv_valuePush.read();
  pv_state.read();
  BOOST_CHECK_EQUAL((T)pv_valuePoll, 45);
  BOOST_CHECK_EQUAL((T)pv_valuePush, 22);
  BOOST_CHECK_EQUAL((T)pv_state, 2);

  // this time the application gets triggered by push2, push is read with
  // readLatest()
  pv_push = 24;
  pv_push.write();
  pv_poll = 46;
  pv_poll.write();
  pv_push2.write();
  test.stepApplication();
  pv_valuePoll.read();
  pv_valuePush.read();
  pv_state.read();
  BOOST_CHECK_EQUAL((T)pv_valuePoll, 46);
  BOOST_CHECK_EQUAL((T)pv_valuePush, 24);
  BOOST_CHECK_EQUAL((T)pv_state, 3);
}
