#include <future>

#define BOOST_TEST_MODULE testExceptionHandling

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/DummyRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ExceptionDevice.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

constexpr char ExceptionDummyCDD1[] = "(ExceptionDummy:1?map=test3.map)";
constexpr char ExceptionDummyCDD2[] = "(ExceptionDummy:2?map=test3.map)";

/* dummy application */

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests

  ctk::DeviceModule dev1{this, ExceptionDummyCDD1};
  ctk::DeviceModule dev2{this, ExceptionDummyCDD2};
  ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/
#if 0
// ************************************************************************************************
// Note: This test partially tests implementation details. It probably should be removed.
// ************************************************************************************************
BOOST_AUTO_TEST_CASE(testDeviceModuleReportExceptionFunction) {
  std::cout << "testDeviceModuleReportExceptionFunction" << std::endl;

  TestApplication app;
  boost::shared_ptr<ExceptionDummy> backend = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));

  app.dev1.connectTo(app.cs, app.cs["MyModule"]("actuator"));
  ctk::TestFacility test;
  test.runApplication();

  auto message = test.getScalar<std::string>(std::string("/Devices/")+ExceptionDummyCDD1+"/message");
  auto status = test.getScalar<int>(std::string("/Devices/")+ExceptionDummyCDD1+"/status");
  auto trigger = test.getScalar<int>("/MyModule/actuator");

  // initially there should be no error set
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) == "");
  BOOST_CHECK(status.readLatest() == 0);

  trigger.write();
  test.stepApplication();

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
    app.testableModeLock("");
    app.dev1.reportException("Some fancy exception text");
    reportExceptionFinished = true;
  });

  // check the error status and that reportException() is still blocking
  trigger.write();
  test.stepApplication();

  message.readLatest();
  status.readLatest();
  BOOST_CHECK_EQUAL(static_cast<std::string>(message),
      "DummyException: This is a test"); // from the ExceptionDummy
  BOOST_CHECK(status == 1);
  BOOST_CHECK(reportExceptionFinished == false);
  BOOST_CHECK(!backend->isOpen());

  // allow to reopen the device successfully, wait until this has hapopened
  backend->throwException = false;
  trigger.write();
  test.stepApplication();
  reportThread.join();

  // the device should now be open again
  BOOST_CHECK(backend->isOpen());

  // check the error status has been cleared
  message.readLatest();
  status.readLatest();
  BOOST_CHECK(static_cast<std::string>(message) == "");
  BOOST_CHECK(status.readLatest() == 0);
}
#endif
/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandling) {
  std::cout << "testExceptionHandling" << std::endl;
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> readbackDummy1(dummyBackend1.get(), "MyModule", "readBack");
  ChimeraTK::DummyRegisterAccessor<int> readbackDummy2(dummyBackend2.get(), "MyModule", "readBack");

  // Connect the whole devices into the control system, and use the control system variable /Device1/MyModule/actuator as trigger for both files.
  // The variable becomes a control system to application variable and writing to it through the test facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs["Device1"]["MyModule"]("actuator"));
  app.dev2.connectTo(app.cs["Device2"], app.cs["Device1"]["MyModule"]("actuator"));

  ctk::TestFacility test;
  test.runApplication();

  app.cs.dump();

  auto message1 = test.getScalar<std::string>(std::string("/Devices/")+ExceptionDummyCDD1+"/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/")+ExceptionDummyCDD1+"/status");
  auto readback1 = test.getScalar<int>("/Device1/MyModule/readBack");
//  auto message2 = test.getScalar<std::string>(std::string("/Devices/")+ExceptionDummyCDD2+"/message");
//  auto status2 = test.getScalar<int>(std::string("/Devices/")+ExceptionDummyCDD2+"/status");
//  auto value2 = test.getScalar<int>(std::string("/Devices/")+ExceptionDummyCDD2+"/FixedPoint/value");
  auto readback2 = test.getScalar<int>("/Device2/MyModule/readBack");
  auto trigger = test.getScalar<int>("Device1/MyModule/actuator");

  readbackDummy1=42;
  readbackDummy2=52;

  // initially there should be no error set
  trigger.write();
  test.stepApplication();
  message1.readLatest();
  status1.readLatest();
  readback1.readLatest();
  readback2.readLatest();
  BOOST_CHECK(static_cast<std::string>(message1) == "");
  BOOST_CHECK(status1 == 0);
  BOOST_CHECK_EQUAL(readback1, 42) ;
  BOOST_CHECK_EQUAL(readback2, 52) ;

  // repeat test a couple of times to make sure it works not only once
  for(size_t i = 0; i < 10; ++i) {
    // enable exception throwing in test device
    readbackDummy1=10+i;
    readbackDummy2=20+i;
    dummyBackend1->throwException = true;
    trigger.write();
    test.stepApplication();
    message1.readLatest();
    status1.readLatest();
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    BOOST_CHECK(!dummyBackend1->isOpen());
    BOOST_CHECK( !readback1.readNonBlocking() ); // no new data for broken device
    BOOST_CHECK( readback2.readNonBlocking() ); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 20+i);
    //the second device must still be functional

    readbackDummy1=30+i;
    readbackDummy2=40+i;

    // Now "cure" the device problem
    dummyBackend1->throwException = false;
    trigger.write();
    test.stepApplication();
    message1.readLatest();
    status1.readLatest();
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(status1, 0);
    BOOST_CHECK(dummyBackend1->isOpen());
    BOOST_CHECK( readback1.readNonBlocking() ); // there is something new in the queue
    BOOST_CHECK( readback2.readNonBlocking() ); // device 2 still works
    BOOST_CHECK_EQUAL(readback1, 30+i);
    BOOST_CHECK_EQUAL(readback2, 40+i);
  }
}
