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

  // works:
  app.dev2.connectTo(app.cs["Device2"], app.cs["Device1"]["MyModule"]("actuator"));
  app.dev1.connectTo(app.cs["Device1"], app.cs["Device1"]["MyModule"]("actuator"));

  // fails: dev2 hangs
  //app.dev1.connectTo(app.cs["Device1"], app.cs["Device1"]["MyModule"]("actuator"));
  //app.dev2.connectTo(app.cs["Device2"], app.cs["Device1"]["MyModule"]("actuator"));

  // fails: exception not caught
  //  app.dev1.connectTo(app.cs["Device1"], app.cs["Device2]["MyModule"]("actuator"));
  //  app.dev2.connectTo(app.cs["Device2"], app.cs["Device2]["MyModule"]("actuator"));

  // fails: exception not caught
  // app.dev2.connectTo(app.cs["Device2"], app.cs["Device2]["MyModule"]("actuator"));
  //  app.dev1.connectTo(app.cs["Device1"], app.cs["Device2]["MyModule"]("actuator"));

  // fails: exception not caught
  //app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  //app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));

  // Do not enable testable mode. The testable mode would block in the wrong place, as the trigger for reading variables
  // of a device in the error state is not being processed until the error state is cleared. We want to test that the
  // second device still works while the first device is in error state, which would be impossible with testable mode
  // enabled. As a consequence, our test logic has to work with timeouts (CHECK_TIMEOUT) etc. instead of the
  // deterministic stepApplication().
  ctk::TestFacility test(false);
  test.runApplication();

  auto message1 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD1 + "/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  auto readback1 = test.getScalar<int>("/Device1/MyModule/readBack");
  auto message2 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD2 + "/message");
  auto status2 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD2 + "/status");
  auto readback2 = test.getScalar<int>("/Device2/MyModule/readBack");

  auto trigger = test.getScalar<int>("Device1/MyModule/actuator");

  readbackDummy1 = 42;
  readbackDummy2 = 52;

  // initially there should be no error set
  trigger.write();
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());
  CHECK_TIMEOUT(readback1.readLatest(), 1000);
  CHECK_TIMEOUT(readback2.readLatest(), 1000);
  BOOST_CHECK(static_cast<std::string>(message1) == "");
  BOOST_CHECK(status1 == 0);
  BOOST_CHECK_EQUAL(readback1, 42);
  BOOST_CHECK_EQUAL(readback2, 52);

  // repeat test a couple of times to make sure it works not only once
  for(size_t i = 0; i < 10; ++i) {
    // enable exception throwing in test device
    readbackDummy1 = 10 + i;
    readbackDummy2 = 20 + i;
    dummyBackend1->throwExceptionRead = true;
    trigger.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    BOOST_CHECK(!readback1.readNonBlocking()); // no new data for broken device
    //the second device must still be functional
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(readback2.readNonBlocking(), 1000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 20 + i);

    // even with device 1 failing the second one must process the data, so send a new trigger
    // before fixing dev1
    readbackDummy2 = 120 + i;
    trigger.write();
    CHECK_TIMEOUT(readback2.readNonBlocking(), 1000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 120 + i);

    readbackDummy1 = 30 + i;
    readbackDummy2 = 40 + i;

    // Now "cure" the device problem
    dummyBackend1->throwExceptionRead = false;
    trigger.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(status1, 0);
    BOOST_CHECK_EQUAL(readback1, 30 + i);
    // there are two more copies in the queue, since the two triggers received during the error state is still
    // processed after recovery
    CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
    BOOST_CHECK_EQUAL(readback1, 30 + i);
    CHECK_TIMEOUT(readback1.readNonBlocking(), 1000);
    BOOST_CHECK_EQUAL(readback1, 30 + i);
    BOOST_CHECK(!readback1.readNonBlocking()); // now the queue should be empty
    // device2
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(readback2.readNonBlocking(), 1000); // device 2 still works
    BOOST_CHECK_EQUAL(readback2, 40 + i);
  }
}
