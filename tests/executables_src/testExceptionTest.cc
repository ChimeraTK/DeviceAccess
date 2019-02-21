#include <future>

#define BOOST_TEST_MODULE testExceptionTest

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <ChimeraTK/Device.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/NDRegisterAccessor.h>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "TestFacility.h"
#include "ControlSystemModule.h"
#include "ExceptionDevice.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/* dummy application */

//bool resetDevice = false;
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }
  void onExceptionResetDevice();

  using Application::makeConnections; // we call makeConnections() manually in the tests to catch exceptions etc.

  void defineConnections() {
    // testableMode = true;
    debugTestableMode();

    dumpConnections();
  } // the setup is done in the tests
  
  ctk::DeviceModule dev{this, "(ExceptionDummy?map=DemoDummy.map)"};
  ctk::ControlSystemModule cs;
};


  
/*********************************************************************************************************************/



BOOST_AUTO_TEST_CASE(testThinkOfAName) {
  
  TestApplication app;
  
  app.dev.connectTo(app.cs);
  //app.dev.deviceError.connectTo(app.cs["DeviceError"]);
  ctk::TestFacility test;
  app.initialise();
  app.run();
  auto message = test.getScalar<std::string>("/Devices.(ExceptionDummy?map=DemoDummy.map)/DeviceError/message");
  auto status = test.getScalar<int>("/Devices.(ExceptionDummy?map=DemoDummy.map)/DeviceError/status");
  
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) == "");
  BOOST_CHECK(status.readLatest() == 0);
  
  boost::shared_ptr<ExceptionDummy> backend = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("(ExceptionDummy?map=DemoDummy.map)"));
      backend->close();
  backend->throwException = true;
  try {
    backend->open();
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::runtime_error&) {
  }
  std::atomic<bool> reportExceptionFinished;
  reportExceptionFinished = false;
  std::thread reportThread([&]{
    app.dev.reportException("exception");
    reportExceptionFinished = true;
  });
  sleep(2);
  
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) != "");
  BOOST_CHECK(status == 1);
  BOOST_CHECK(reportExceptionFinished == false);
  backend->throwException = false;
  reportThread.join();
  
  BOOST_CHECK(reportExceptionFinished == true);
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) == "");
  BOOST_CHECK(status.readLatest() == 0);
  
  
  
  
  
}
