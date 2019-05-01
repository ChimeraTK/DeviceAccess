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
#ifdef _0
BOOST_AUTO_TEST_CASE(testExceptionHandlingRead) {
  std::cout << "testExceptionHandlingRead" << std::endl;
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> readbackDummy1(dummyBackend1.get(), "MyModule", "readBack");
  ChimeraTK::DummyRegisterAccessor<int> readbackDummy2(dummyBackend2.get(), "MyModule", "readBack");

  // Connect the whole devices into the control system, and use the control system variable /trigger as trigger for
  // both devices. The variable becomes a control system to application variable and writing to it through the test
  // facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));

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

  auto trigger = test.getScalar<int>("trigger");

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
  for(size_t i = 0; i < 3; ++i) {
    // enable exception throwing in test device 1
    readbackDummy1 = 10 + i;
    readbackDummy2 = 20 + i;
    dummyBackend1->throwExceptionRead = true;
    trigger.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    BOOST_CHECK(!readback1.readNonBlocking()); // no new data for broken device
    // the second device must still be functional
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

    // Now "cure" the device problem
    readbackDummy1 = 30 + i;
    readbackDummy2 = 40 + i;
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

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandlingWrite) {
  std::cout << "testExceptionHandlingWrite" << std::endl;
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> actuatorDummy1(dummyBackend1.get(), "MyModule", "actuator");
  ChimeraTK::DummyRegisterAccessor<int> actuatorDummy2(dummyBackend2.get(), "MyModule", "actuator");

  // Connect the whole devices into the control system, and use the control system variable /trigger as trigger for
  // both devices. The variable becomes a control system to application variable and writing to it through the test
  // facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));

  // Do not enable testable mode. The testable mode would block in the wrong place, as the trigger for reading variables
  // of a device in the error state is not being processed until the error state is cleared. We want to test that the
  // second device still works while the first device is in error state, which would be impossible with testable mode
  // enabled. As a consequence, our test logic has to work with timeouts (CHECK_TIMEOUT) etc. instead of the
  // deterministic stepApplication().
  ctk::TestFacility test(false);
  test.runApplication();

  auto message1 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD1 + "/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  auto actuator1 = test.getScalar<int>("/Device1/MyModule/actuator");
  auto message2 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD2 + "/message");
  auto status2 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD2 + "/status");
  auto actuator2 = test.getScalar<int>("/Device2/MyModule/actuator");

  auto trigger = test.getScalar<int>("trigger");

  // initially there should be no error set
  actuator1 = 29;
  actuator1.write();
  actuator2 = 39;
  actuator2.write();
  BOOST_CHECK(!message1.readLatest());
  BOOST_CHECK(!status1.readLatest());
  CHECK_TIMEOUT(actuatorDummy1 == 29, 1000);
  CHECK_TIMEOUT(actuatorDummy2 == 39, 1000);
  BOOST_CHECK(static_cast<std::string>(message1) == "");
  BOOST_CHECK(status1 == 0);

  // repeat test a couple of times to make sure it works not only once
  for(size_t i = 0; i < 3; ++i) {
    // enable exception throwing in test device 1
    dummyBackend1->throwExceptionWrite = true;
    actuator1 = 30 + i;
    actuator1.write();
    actuator2 = 40 + i;
    actuator2.write();
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    BOOST_CHECK(static_cast<std::string>(message1) != "");
    BOOST_CHECK_EQUAL(status1, 1);
    usleep(10000);                                  // 10ms wait time so potential wrong values could have propagated
    BOOST_CHECK(actuatorDummy1 == int(30 + i - 1)); // write not done for broken device
    // the second device must still be functional
    BOOST_CHECK(!message2.readNonBlocking());
    BOOST_CHECK(!status2.readNonBlocking());
    CHECK_TIMEOUT(actuatorDummy2 == int(40 + i), 1000); // device 2 still works

    // even with device 1 failing the second one must process the data, so send a new data before fixing dev1
    actuator2 = 120 + i;
    actuator2.write();
    CHECK_TIMEOUT(actuatorDummy2 == int(120 + i), 1000); // device 2 still works

    // Now "cure" the device problem
    dummyBackend1->throwExceptionWrite = false;
    CHECK_TIMEOUT(message1.readLatest(), 1000);
    CHECK_TIMEOUT(status1.readLatest(), 1000);
    CHECK_TIMEOUT(actuatorDummy1 == int(30 + i), 1000); // write is now complete
    BOOST_CHECK_EQUAL(static_cast<std::string>(message1), "");
    BOOST_CHECK_EQUAL(status1, 0);
  }
}
#endif
/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandlingOpen) {
  std::cout << "testExceptionHandlingOpen" << std::endl;
  TestApplication app;
  boost::shared_ptr<ExceptionDummy> dummyBackend1 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD1));
  boost::shared_ptr<ExceptionDummy> dummyBackend2 = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(ExceptionDummyCDD2));

  ChimeraTK::DummyRegisterAccessor<int> readbackDummy1(dummyBackend1.get(), "MyModule", "readBack");
  ChimeraTK::DummyRegisterAccessor<int> readbackDummy2(dummyBackend2.get(), "MyModule", "readBack");

  // Connect the whole devices into the control system, and use the control system variable /trigger as trigger for
  // both devices. The variable becomes a control system to application variable and writing to it through the test
  // facility is generating the triggers.
  app.dev1.connectTo(app.cs["Device1"], app.cs("trigger", typeid(int), 1));
  //app.dev2.connectTo(app.cs["Device2"], app.cs("trigger"));
  app.dev2.connectTo(app.cs["Device2"], app.cs("trigger2", typeid(int), 1));

  // Do not enable testable mode. The testable mode would block in the wrong place, as the trigger for reading variables
  // of a device in the error state is not being processed until the error state is cleared. We want to test that the
  // second device still works while the first device is in error state, which would be impossible with testable mode
  // enabled. As a consequence, our test logic has to work with timeouts (CHECK_TIMEOUT) etc. instead of the
  // deterministic stepApplication().
  ctk::TestFacility test(false);
  dummyBackend1->throwExceptionOpen = true;
  test.runApplication();

  auto message1 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD1 + "/message");
  auto status1 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD1 + "/status");
  auto readback1 = test.getScalar<int>("/Device1/MyModule/readBack");
  auto message2 = test.getScalar<std::string>(std::string("/Devices/") + ExceptionDummyCDD2 + "/message");
  auto status2 = test.getScalar<int>(std::string("/Devices/") + ExceptionDummyCDD2 + "/status");
  auto readback2 = test.getScalar<int>("/Device2/MyModule/readBack");

  auto trigger = test.getScalar<int>("trigger");
  auto trigger2 = test.getScalar<int>("trigger2");
 
  readbackDummy1 = 100;
  trigger.write();
  CHECK_TIMEOUT(status1.readLatest(), 2000);
  //CHECK_TIMEOUT(message1.readLatest(), 2000);
  //CHECK_TIMEOUT(readback1.readNonBlocking(), 2000);
  //BOOST_CHECK_THROW(readback1.readNonBlocking(), ChimeraTK::runtime_error);

  // even with device 1 failing the second one must process the data, so send a new trigger
  // before fixing dev1
  readbackDummy2 = 120;
  trigger2.write();
  CHECK_TIMEOUT(readback2.readNonBlocking(), 2000); // device 2 still works
  BOOST_CHECK_EQUAL(readback2, 120);

  dummyBackend1->throwExceptionOpen = false;
}
