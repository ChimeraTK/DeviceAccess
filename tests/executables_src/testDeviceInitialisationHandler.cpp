//#include <future>

#define BOOST_TEST_MODULE testDeviceInitialisationHandler

//#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/test_case_template.hpp>

#include "Application.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
//#include "ScalarAccessor.h"
#include "TestFacility.h"
//#include "StatusMonitor.h"

#include <ChimeraTK/Device.h>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

void initialiseReg1(ctk::DeviceModule * dev){
    dev->device.write<int32_t>("/REG1",42);
}

/* dummy application */
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests
  ctk::ControlSystemModule cs;
  ctk::DeviceModule dev{this, "(dummy?map=test.map)",&initialiseReg1};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBasicInitialisation) {
  std::cout << "testBasicInitialisation" << std::endl;
  TestApplication app;

  app.dev.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  ctk::Device dummy;
  dummy.open("(dummy?map=test.map)");
  BOOST_CHECK_EQUAL(dummy.read<int32_t>("REG1"),42);

//  auto error = test.getScalar<double_t>(std::string("/MAX_MONITOR.ERROR.THRESHOLD"));
//  error = 50.1;
//  error.write();
//  test.stepApplication();

//  auto watch = test.getScalar<double_t>(std::string("/WATCH"));
//  watch = 40.1;
//  watch.write();
//  test.stepApplication();

//  auto status = test.getScalar<uint16_t>(std::string("/STATUS"));
//  status.readLatest();

//  //should be in OK state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

//  //set watch value exceeding warning level
//  watch = 46.1;
//  watch.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in WARNING state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

//  //set watch value exceeding error level
//  watch = 51.1;
//  watch.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in ERROR state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

//  //increase error value greater than watch
//  error = 60.1;
//  error.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in WARNING state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

//  //increase warning value greater than watch
//  warning = 55.1;
//  warning.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in OK state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

//  //set watch value exceeding error level
//  watch = 65.1;
//  watch.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in ERROR state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

//  //decrease watch value lower than error level but still greater than warning level
//  watch = 58.1;
//  watch.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in WARNING state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

//  //decrease watch value lower than warning level
//  watch = 54.1;
//  watch.write();
//  test.stepApplication();
//  status.readLatest();
//  //should be in OK state.
//  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);
}

