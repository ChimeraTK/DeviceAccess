#include <future>

#define BOOST_TEST_MODULE testExceptionHandling

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ExceptionDevice.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/* dummy application */

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests

  ctk::DeviceModule dev{this, "(ExceptionDummy?map=test.map)"};
  ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDeviceModuleReportExceptionFunction) {
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> backend = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("(ExceptionDummy?map=test.map)"));

  app.dev.connectTo(app.cs);
  ctk::TestFacility test;
  app.initialise();
  app.run();
  auto message = test.getScalar<std::string>("/Devices/(ExceptionDummy?map=test.map)/message");
  auto status = test.getScalar<int>("/Devices/(ExceptionDummy?map=test.map)/status");

  // initially there should be no error set
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) == "");
  BOOST_CHECK(status.readLatest() == 0);

  // close the device, reopening it will throw an exception
  backend->close();
  backend->throwException = true;

  // test the error injection capability of our ExceptionDummy
  try {
    backend->open();
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::runtime_error&) {
  }

  // report exception to the DeviceModule: it should try reopening the device
  // but fail
  std::atomic<bool> reportExceptionFinished;
  reportExceptionFinished = false;
  std::thread reportThread([&] { // need to launch in background, reportException() blocks
    app.dev.reportException("Some fancy exception text");
    reportExceptionFinished = true;
  });

  // check the error status and that reportException() is still blocking
  sleep(2);
  message.readLatest();
  status.readLatest();
  BOOST_CHECK_EQUAL(static_cast<std::string>(message),
      "DummyException: This is a test"); // from the ExceptionDummy
  BOOST_CHECK(status == 1);
  BOOST_CHECK(reportExceptionFinished == false);
  BOOST_CHECK(!backend->isOpen());

  // allow to reopen the device successfully, wait until this has hapopened
  backend->throwException = false;
  reportThread.join();

  // the device should now be open again
  BOOST_CHECK(backend->isOpen());

  // check the error status has been cleared
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) == "");
  BOOST_CHECK(status.readLatest() == 0);
}
