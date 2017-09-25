/*
 * testDeviceAccessors.cc
 *
 *  Created on: Jun 22, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testDeviceAccessors

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <mtca4u/BackendFactory.h>
#include <mtca4u/NDRegisterAccessor.h>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"

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
struct TestModule : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPollInput<T> consumingPoll{this, "consumingPoll", "MV/m", "Description"};

    ctk::ScalarOutput<T> feedingToDevice{this, "feedingToDevice", "MV/m", "Description"};

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
    TestApplication() : Application("test suite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    using Application::deviceMap;           // expose the device map for the tests
    void defineConnections() {}             // the setup is done in the tests

    TestModule<T> testModule{this,"testModule", "The test module"};
    ctk::DeviceModule devMymodule{"Dummy0","MyModule"};
    ctk::DeviceModule dev{"Dummy0"};
};

/*********************************************************************************************************************/
/* test feeding a scalar to a device */

BOOST_AUTO_TEST_CASE_TEMPLATE( testFeedToDevice, T, test_types ) {
  std::cout << "testFeedToDevice" << std::endl;

  mtca4u::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.testModule.feedingToDevice >> app.devMymodule("actuator");
  app.initialise();

  boost::shared_ptr<mtca4u::DeviceBackend> backend = app.deviceMap["Dummy0"];
  auto regacc = backend->getRegisterAccessor<int>("/MyModule/actuator",1,0,{});

  regacc->accessData(0) = 0;
  app.testModule.feedingToDevice = 42;
  app.testModule.feedingToDevice.write();
  regacc->read();
  BOOST_CHECK(regacc->accessData(0) == 42);
  app.testModule.feedingToDevice = 120;
  regacc->read();
  BOOST_CHECK(regacc->accessData(0) == 42);
  app.testModule.feedingToDevice.write();
  regacc->read();
  BOOST_CHECK(regacc->accessData(0) == 120);

}

/*********************************************************************************************************************/
/* test consuming a scalar from a device */

BOOST_AUTO_TEST_CASE_TEMPLATE( testConsumeFromDevice, T, test_types ) {
  std::cout << "testConsumeFromDevice" << std::endl;

  mtca4u::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.dev("/MyModule/actuator") >> app.testModule.consumingPoll;
  app.initialise();

  boost::shared_ptr<mtca4u::DeviceBackend> backend = app.deviceMap["Dummy0"];
  auto regacc = backend->getRegisterAccessor<int>("/MyModule/actuator",1,0,{});

  // single theaded test only, since read() does not block in this case
  app.testModule.consumingPoll = 0;
  regacc->accessData(0) = 42;
  regacc->write();
  BOOST_CHECK(app.testModule.consumingPoll == 0);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  regacc->accessData(0) = 120;
  regacc->write();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK( app.testModule.consumingPoll == 120 );
  app.testModule.consumingPoll.read();
  BOOST_CHECK( app.testModule.consumingPoll == 120 );
  app.testModule.consumingPoll.read();
  BOOST_CHECK( app.testModule.consumingPoll == 120 );

}

