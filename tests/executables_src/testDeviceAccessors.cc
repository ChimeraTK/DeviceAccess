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
class TestModule : public ctk::ApplicationModule {
  public:

    SCALAR_INPUT(T, consumingPoll, "MV/m", ctk::UpdateMode::poll);

    SCALAR_OUTPUT(T, feedingToDevice, "MV/m");

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

class TestApplication : public ctk::Application {
  public:
    TestApplication() : Application("test suite") {}

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    using Application::deviceMap;           // expose the device map for the tests
    void initialise() {}                    // the setup is done in the tests
};

/*********************************************************************************************************************/
/* test feeding a scalar to a device */

BOOST_AUTO_TEST_CASE_TEMPLATE( testFeedToDevice, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication app;
  TestModule<T> testModule;
  ctk::DeviceModule dev{"Dummy0","MyModule"};

  testModule.feedingToDevice >> dev("Variable");
  app.makeConnections();

  auto regacc = app.deviceMap["Dummy0"]->getRegisterAccessor<int>("/MyModule/Variable",1,0,{});

  regacc->accessData(0) = 0;
  testModule.feedingToDevice = 42;
  testModule.feedingToDevice.write();
  regacc->read();
  BOOST_CHECK(regacc->accessData(0) == 42);
  testModule.feedingToDevice = 120;
  regacc->read();
  BOOST_CHECK(regacc->accessData(0) == 42);
  testModule.feedingToDevice.write();
  regacc->read();
  BOOST_CHECK(regacc->accessData(0) == 120);

}

/*********************************************************************************************************************/
/* test consuming a scalar from a device */

BOOST_AUTO_TEST_CASE_TEMPLATE( testConsumeFromDevice, T, test_types ) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

  TestApplication app;
  TestModule<T> testModule;
  ctk::DeviceModule dev{"Dummy0"};

  dev("/MyModule/Variable") >> testModule.consumingPoll;
  app.makeConnections();

  auto regacc = app.deviceMap["Dummy0"]->getRegisterAccessor<int>("/MyModule/Variable",1,0,{});

  // single theaded test only, since read() does not block in this case
  testModule.consumingPoll = 0;
  regacc->accessData(0) = 42;
  regacc->write();
  BOOST_CHECK(testModule.consumingPoll == 0);
  testModule.consumingPoll.read();
  BOOST_CHECK(testModule.consumingPoll == 42);
  testModule.consumingPoll.read();
  BOOST_CHECK(testModule.consumingPoll == 42);
  testModule.consumingPoll.read();
  BOOST_CHECK(testModule.consumingPoll == 42);
  regacc->accessData(0) = 120;
  regacc->write();
  BOOST_CHECK(testModule.consumingPoll == 42);
  testModule.consumingPoll.read();
  BOOST_CHECK( testModule.consumingPoll == 120 );
  testModule.consumingPoll.read();
  BOOST_CHECK( testModule.consumingPoll == 120 );
  testModule.consumingPoll.read();
  BOOST_CHECK( testModule.consumingPoll == 120 );

}

