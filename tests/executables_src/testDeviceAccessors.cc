/*
 * testDeviceAccessors.cc
 *
 *  Created on: Jun 22, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testDeviceAccessors

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, float, double> test_types;

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

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

template<typename T>
struct TestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPollInput<T> consumingPoll{this, "consumingPoll", "MV/m", "Description"};

  ctk::ScalarPushInput<T> consumingPush{this, "consumingPush", "MV/m", "Description"};
  ctk::ScalarPushInput<T> consumingPush2{this, "consumingPush2", "MV/m", "Description"};

  ctk::ScalarOutput<T> feedingToDevice{this, "feedingToDevice", "MV/m", "Description"};

  void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

template<typename T>
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  using Application::deviceMap;       // expose the device map for the tests
  using Application::makeConnections; // we call makeConnections() manually in
                                      // the tests to catch exceptions etc.
  using Application::networkList;     // expose network list to check merging
                                      // networks
  void defineConnections() {}         // the setup is done in the tests

  TestModule<T> testModule{this, "testModule", "The test module"};
  ctk::DeviceModule dev{this, "Dummy0"};

  // note: direct device-to-controlsystem connections are tested in
  // testControlSystemAccessors!
};

/*********************************************************************************************************************/
/* test feeding a scalar to a device */

BOOST_AUTO_TEST_CASE_TEMPLATE(testFeedToDevice, T, test_types) {
  std::cout << "testFeedToDevice" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.testModule.feedingToDevice >> app.dev["MyModule"]("actuator");

  ctk::TestFacility test;
  app.run();
  ChimeraTK::Device dev;
  dev.open("Dummy0");
  auto regacc = dev.getScalarRegisterAccessor<int>("/MyModule/actuator");

  regacc = 0;
  app.testModule.feedingToDevice = 42;
  app.testModule.feedingToDevice.write();
  regacc.read();
  BOOST_CHECK(regacc == 42);
  app.testModule.feedingToDevice = 120;
  regacc.read();
  BOOST_CHECK(regacc == 42);
  app.testModule.feedingToDevice.write();
  regacc.read();
  BOOST_CHECK(regacc == 120);
}

/*********************************************************************************************************************/
/* test feeding a scalar to two different device registers */

BOOST_AUTO_TEST_CASE_TEMPLATE(testFeedToDeviceFanOut, T, test_types) {
  std::cout << "testFeedToDeviceFanOut" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.testModule.feedingToDevice >> app.dev["MyModule"]("actuator") >> app.dev["MyModule"]("readBack");
  ctk::TestFacility test;
  app.run();
  ChimeraTK::Device dev;
  dev.open("Dummy0");

  auto regac = dev.getScalarRegisterAccessor<int>("/MyModule/actuator");
  auto regrb = dev.getScalarRegisterAccessor<int>("/MyModule/readBack");

  regac = 0;
  regrb = 0;
  app.testModule.feedingToDevice = 42;
  app.testModule.feedingToDevice.write();
  regac.read();
  BOOST_CHECK(regac == 42);
  regrb.read();
  BOOST_CHECK(regrb == 42);
  app.testModule.feedingToDevice = 120;
  regac.read();
  BOOST_CHECK(regac == 42);
  regrb.read();
  BOOST_CHECK(regrb == 42);
  app.testModule.feedingToDevice.write();
  regac.read();
  BOOST_CHECK(regac == 120);
  regrb.read();
  BOOST_CHECK(regrb == 120);
}

/*********************************************************************************************************************/
/* test consuming a scalar from a device */

BOOST_AUTO_TEST_CASE_TEMPLATE(testConsumeFromDevice, T, test_types) {
  std::cout << "testConsumeFromDevice" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.dev("/MyModule/actuator") >> app.testModule.consumingPoll;
  ctk::TestFacility test;
  app.run();
  ChimeraTK::Device dev;
  dev.open("Dummy0");
  auto regacc = dev.getScalarRegisterAccessor<int>("/MyModule/actuator");

  // single theaded test only, since read() does not block in this case
  app.testModule.consumingPoll = 0;
  regacc = 42;
  regacc.write();
  BOOST_CHECK(app.testModule.consumingPoll == 0);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  regacc = 120;
  regacc.write();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPoll == 120);
}

/*********************************************************************************************************************/
/* test consuming a scalar from a device with a ConsumingFanOut (i.e. one
 * poll-type consumer and several push-type consumers). */

BOOST_AUTO_TEST_CASE_TEMPLATE(testConsumingFanOut, T, test_types) {
  std::cout << "testConsumingFanOut" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.dev("/MyModule/actuator") >> app.testModule.consumingPoll >> app.testModule.consumingPush >>
      app.testModule.consumingPush2;
  ctk::TestFacility test;
  app.run();
  ChimeraTK::Device dev;
  dev.open("Dummy0");
  auto regacc = dev.getScalarRegisterAccessor<int>("/MyModule/actuator");

  // single theaded test only, since read() does not block in this case
  app.testModule.consumingPoll = 0;
  regacc = 42;
  regacc.write();

  BOOST_CHECK(app.testModule.consumingPoll == 0);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush == 0);
  BOOST_CHECK(app.testModule.consumingPush2 == 0);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  regacc = 120;
  regacc.write();
  BOOST_CHECK(app.testModule.consumingPoll == 42);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  BOOST_CHECK(app.testModule.consumingPush == 120);
  BOOST_CHECK(app.testModule.consumingPush2 == 120);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  BOOST_CHECK(app.testModule.consumingPush == 120);
  BOOST_CHECK(app.testModule.consumingPush2 == 120);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
  app.testModule.consumingPoll.read();
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == true);
  BOOST_CHECK(app.testModule.consumingPoll == 120);
  BOOST_CHECK(app.testModule.consumingPush == 120);
  BOOST_CHECK(app.testModule.consumingPush2 == 120);
  BOOST_CHECK(app.testModule.consumingPush.readNonBlocking() == false);
  BOOST_CHECK(app.testModule.consumingPush2.readNonBlocking() == false);
}

/*********************************************************************************************************************/
/* test merged networks (optimisation done in
 * Application::optimiseConnections()) */

BOOST_AUTO_TEST_CASE_TEMPLATE(testMergedNetworks, T, test_types) {
  std::cout << "testMergedNetworks" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  // we abuse "feedingToDevice" as trigger here...
  app.dev("/MyModule/actuator")[app.testModule.feedingToDevice] >> app.testModule.consumingPush;
  app.dev("/MyModule/actuator")[app.testModule.feedingToDevice] >> app.testModule.consumingPush2;

  // check that we have two separate networks for both connections
  size_t nDeviceFeeders = 0;
  for(auto& net : app.networkList) {
    if(net.getFeedingNode().getType() == ctk::NodeType::Device) nDeviceFeeders++;
  }
  BOOST_CHECK_EQUAL(nDeviceFeeders, 2);

  // the optimisation to test takes place here
  ctk::TestFacility test;

  // check we are left with just one network fed by the device
  nDeviceFeeders = 0;
  for(auto& net : app.networkList) {
    if(net.getFeedingNode().getType() == ctk::NodeType::Device) nDeviceFeeders++;
  }
  BOOST_CHECK_EQUAL(nDeviceFeeders, 1);

  // run the application to see if everything still behaves as expected
  app.run();

  ChimeraTK::Device dev;
  dev.open("Dummy0");
  auto regacc = dev.getScalarRegisterAccessor<int>("/MyModule/actuator");

  // single theaded test only, since read() does not block in this case
  app.testModule.consumingPush = 0;
  app.testModule.consumingPush2 = 0;
  regacc = 42;
  regacc.write();
  BOOST_CHECK(app.testModule.consumingPush == 0);
  BOOST_CHECK(app.testModule.consumingPush2 == 0);
  app.testModule.feedingToDevice.write();
  app.testModule.consumingPush.read();
  app.testModule.consumingPush2.read();
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  regacc = 120;
  regacc.write();
  BOOST_CHECK(app.testModule.consumingPush == 42);
  BOOST_CHECK(app.testModule.consumingPush2 == 42);
  app.testModule.feedingToDevice.write();
  app.testModule.consumingPush.read();
  app.testModule.consumingPush2.read();
  BOOST_CHECK(app.testModule.consumingPush == 120);
  BOOST_CHECK(app.testModule.consumingPush2 == 120);
}

/*********************************************************************************************************************/
/* test feeding a constant to a device register. */

BOOST_AUTO_TEST_CASE_TEMPLATE(testConstantToDevice, T, test_types) {
  std::cout << "testConstantToDevice" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  ctk::VariableNetworkNode::makeConstant<T>(true, 18) >> app.dev("/MyModule/actuator");
  ctk::TestFacility test;
  test.runApplication();

  ChimeraTK::Device dev;
  dev.open("Dummy0");

  CHECK_TIMEOUT(dev.read<T>("/MyModule/actuator") == 18, 3000);
}

/*********************************************************************************************************************/
/* test feeding a constant to a device register with a fan out. */

BOOST_AUTO_TEST_CASE_TEMPLATE(testConstantToDeviceFanOut, T, test_types) {
  std::cout << "testConstantToDeviceFanOut" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  ctk::VariableNetworkNode::makeConstant<T>(true, 20) >> app.dev("/MyModule/actuator") >> app.dev("/MyModule/readBack");
  ctk::TestFacility test;
  test.runApplication();

  ChimeraTK::Device dev;
  dev.open("Dummy0");

  CHECK_TIMEOUT(dev.read<T>("/MyModule/actuator") == 20, 3000);
  CHECK_TIMEOUT(dev.read<T>("/MyModule/readBack") == 20, 3000);
}

/*********************************************************************************************************************/
/* test subscript operator of DeviceModule */

BOOST_AUTO_TEST_CASE_TEMPLATE(testDeviceModuleSubscriptOp, T, test_types) {
  std::cout << "testDeviceModuleSubscriptOp" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<T> app;

  app.testModule.feedingToDevice >> app.dev["MyModule"]("actuator");
  ctk::TestFacility test;
  app.run();
  ChimeraTK::Device dev;
  dev.open("Dummy0");
  auto regacc = dev.getScalarRegisterAccessor<int>("/MyModule/actuator");

  regacc = 0;
  app.testModule.feedingToDevice = 42;
  app.testModule.feedingToDevice.write();
  regacc.read();
  BOOST_CHECK(regacc == 42);
  app.testModule.feedingToDevice = 120;
  regacc.read();
  BOOST_CHECK(regacc == 42);
  app.testModule.feedingToDevice.write();
  regacc.read();
  BOOST_CHECK(regacc == 120);
}

/*********************************************************************************************************************/
/* test DeviceModule::virtualise()  (trivial implementation) */

BOOST_AUTO_TEST_CASE(testDeviceModuleVirtuallise) {
  std::cout << "testDeviceModuleVirtuallise" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication<int> app;

  app.testModule.feedingToDevice >> app.dev.virtualise()["MyModule"]("actuator");

  ctk::TestFacility test;

  BOOST_CHECK(&(app.dev.virtualise()) == &(app.dev));
}

/*********************************************************************************************************************/
/* test connectTo() */

template<typename T>
struct TestModule2 : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarOutput<T> actuator{this, "actuator", "MV/m", "Description"};
  ctk::ScalarPollInput<T> readback{this, "readBack", "MV/m", "Description"};

  void mainLoop() {}
};

template<typename T>
struct Deeper : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  struct Hierarchies : ctk::VariableGroup {
    using ctk::VariableGroup ::VariableGroup;
    struct Need : ctk::VariableGroup {
      using ctk::VariableGroup ::VariableGroup;
      ctk::ScalarPollInput<T> tests{this, "tests", "MV/m", "Description"};
    } need{this, "need", ""};
    ctk::ScalarOutput<T> also{this, "also", "MV/m", "Description", {"ALSO"}};
  } hierarchies{this, "hierarchies", ""};

  void mainLoop() {}
};

template<typename T>
struct TestApplication2 : public ctk::Application {
  TestApplication2() : Application("testSuite") {}
  ~TestApplication2() { shutdown(); }

  using Application::deviceMap;       // expose the device map for the tests
  using Application::makeConnections; // we call makeConnections() manually in
                                      // the tests to catch exceptions etc.
  using Application::networkList;     // expose network list to check merging
                                      // networks
  void defineConnections() {}         // the setup is done in the tests

  TestModule2<T> testModule{this, "MyModule", "The test module"};
  Deeper<T> deeper{this, "Deeper", ""};
  ctk::DeviceModule dev{this, "Dummy0"};
};

BOOST_AUTO_TEST_CASE_TEMPLATE(testConnectTo, T, test_types) {
  std::cout << "testConnectTo" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication2<int> app;
  app.testModule.connectTo(app.dev["MyModule"]);
  app.deeper.hierarchies.need.connectTo(app.dev["Deeper"]["hierarchies"]["need"]);
  app.deeper.hierarchies.findTag("ALSO").connectTo(app.dev["Deeper"]["hierarchies"]);

  ctk::TestFacility test;
  test.runApplication();

  ctk::Device dev;
  dev.open("Dummy0");
  auto actuator = dev.getScalarRegisterAccessor<T>("MyModule/actuator");
  auto readback = dev.getScalarRegisterAccessor<T>("MyModule/readBack");
  auto tests = dev.getScalarRegisterAccessor<T>("Deeper/hierarchies/need/tests");
  auto also = dev.getScalarRegisterAccessor<T>("Deeper/hierarchies/also");

  app.testModule.actuator = 42;
  app.testModule.actuator.write();
  actuator.read();
  BOOST_CHECK_EQUAL(T(actuator), 42);

  app.testModule.actuator = 12;
  app.testModule.actuator.write();
  actuator.read();
  BOOST_CHECK_EQUAL(T(actuator), 12);

  readback = 120;
  readback.write();
  app.testModule.readback.read();
  BOOST_CHECK_EQUAL(T(app.testModule.readback), 120);

  readback = 66;
  readback.write();
  app.testModule.readback.read();
  BOOST_CHECK_EQUAL(T(app.testModule.readback), 66);

  tests = 120;
  tests.write();
  app.deeper.hierarchies.need.tests.read();
  BOOST_CHECK_EQUAL(T(app.deeper.hierarchies.need.tests), 120);

  tests = 66;
  tests.write();
  app.deeper.hierarchies.need.tests.read();
  BOOST_CHECK_EQUAL(T(app.deeper.hierarchies.need.tests), 66);

  app.deeper.hierarchies.also = 42;
  app.deeper.hierarchies.also.write();
  also.read();
  BOOST_CHECK_EQUAL(T(also), 42);

  app.deeper.hierarchies.also = 12;
  app.deeper.hierarchies.also.write();
  also.read();
  BOOST_CHECK_EQUAL(T(also), 12);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testConnectTo2, T, test_types) {
  std::cout << "testConnectTo2" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");

  TestApplication2<int> app;
  app.findTag(".*").connectTo(app.dev);

  ctk::TestFacility test;
  test.runApplication();

  ctk::Device dev;
  dev.open("Dummy0");
  auto actuator = dev.getScalarRegisterAccessor<T>("MyModule/actuator");
  auto readback = dev.getScalarRegisterAccessor<T>("MyModule/readBack");
  auto tests = dev.getScalarRegisterAccessor<T>("Deeper/hierarchies/need/tests");
  auto also = dev.getScalarRegisterAccessor<T>("Deeper/hierarchies/also");

  app.testModule.actuator = 42;
  app.testModule.actuator.write();
  actuator.read();
  BOOST_CHECK_EQUAL(T(actuator), 42);

  app.testModule.actuator = 12;
  app.testModule.actuator.write();
  actuator.read();
  BOOST_CHECK_EQUAL(T(actuator), 12);

  readback = 120;
  readback.write();
  app.testModule.readback.read();
  BOOST_CHECK_EQUAL(T(app.testModule.readback), 120);

  readback = 66;
  readback.write();
  app.testModule.readback.read();
  BOOST_CHECK_EQUAL(T(app.testModule.readback), 66);

  tests = 120;
  tests.write();
  app.deeper.hierarchies.need.tests.read();
  BOOST_CHECK_EQUAL(T(app.deeper.hierarchies.need.tests), 120);

  tests = 66;
  tests.write();
  app.deeper.hierarchies.need.tests.read();
  BOOST_CHECK_EQUAL(T(app.deeper.hierarchies.need.tests), 66);

  app.deeper.hierarchies.also = 42;
  app.deeper.hierarchies.also.write();
  also.read();
  BOOST_CHECK_EQUAL(T(also), 42);

  app.deeper.hierarchies.also = 12;
  app.deeper.hierarchies.also.write();
  also.read();
  BOOST_CHECK_EQUAL(T(also), 12);
}
