#include <future>

#define BOOST_TEST_MODULE testPropagateDataFaultFlag

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
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "TestFacility.h"

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

/* dummy application */

struct TestModule1 : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPushInput<int> i1{this, "i1", "", ""};
  ctk::ArrayPushInput<int> i2{this, "i2", "", 2, ""};
  ctk::ScalarPushInputWB<int> i3{this, "i3", "", ""};
  ctk::ScalarOutput<int> o1{this, "o1", "", ""};
  ctk::ArrayOutput<int> o2{this, "o2", "", 2, ""};
  void mainLoop() override {
    auto group = readAnyGroup();
    while(true) {
      group.readAny();
      if(i3 > 10) {
        i3 = 10;
        i3.write();
      }
      o1 = int(i1);
      o2[0] = i2[0];
      o2[1] = i2[1];
      o1.write();
      o2.write();
    }
  }
};

struct TestApplication1 : ctk::Application {
  TestApplication1() : Application("testSuite") {}
  ~TestApplication1() { shutdown(); }

  void defineConnections() { t1.connectTo(cs); }

  TestModule1 t1{this, "t1", ""};
  ctk::ControlSystemModule cs;
};

struct TestApplication2 : ctk::Application {
  TestApplication2() : Application("testSuite") {}
  ~TestApplication2() { shutdown(); }

  void defineConnections() {
    t1.connectTo(cs["A"]);
    t1.connectTo(cs["B"]);
  }

  TestModule1 t1{this, "t1", ""};
  ctk::ControlSystemModule cs;
};
/*********************************************************************************************************************/

// first test without FanOuts of any kind
BOOST_AUTO_TEST_CASE(testDirectConnections) {
  std::cout << "testDirectConnections" << std::endl;
  TestApplication1 app;
  ctk::TestFacility test;

  auto i1 = test.getScalar<int>("i1");
  auto i2 = test.getArray<int>("i2");
  auto i3 = test.getScalar<int>("i3");
  auto o1 = test.getScalar<int>("o1");
  auto o2 = test.getArray<int>("o2");

  test.runApplication();

  // test if fault flag propagates to all outputs
  i1 = 1;
  i1.setDataValidity(ctk::DataValidity::faulty);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 1);
  BOOST_CHECK_EQUAL(o2[0], 0);
  BOOST_CHECK_EQUAL(o2[1], 0);

  // write another value but keep fault flag
  i1 = 42;
  BOOST_CHECK(i1.dataValidity() == ctk::DataValidity::faulty);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 42);
  BOOST_CHECK_EQUAL(o2[0], 0);
  BOOST_CHECK_EQUAL(o2[1], 0);

  // a write on the ok variable should not clear the flag
  i2[0] = 10;
  i2[1] = 11;
  BOOST_CHECK(i2.dataValidity() == ctk::DataValidity::ok);
  i2.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 42);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);

  // the return channel should also receive the flag
  BOOST_CHECK(i3.readNonBlocking() == false);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::ok);
  i3 = 20;
  i3.write();
  test.stepApplication();
  o1.read();
  o2.read();
  i3.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 42);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // clear the flag on i1, i3 will keep it for now (we have received it there and not yet sent it out!)
  i1 = 3;
  i1.setDataValidity(ctk::DataValidity::ok);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(i3.readNonBlocking() == false);
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(o1), 3);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // send two data fault flags. both need to be cleared before the outputs go back to ok
  i1 = 120;
  i1.setDataValidity(ctk::DataValidity::faulty);
  i1.write();
  i3 = 121;
  i3.write();
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  test.stepApplication();
  o1.readLatest();
  o2.readLatest();
  i3.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 120);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // clear first flag
  i1 = 122;
  i1.setDataValidity(ctk::DataValidity::ok);
  i1.write();
  test.stepApplication();
  o1.read();
  o2.read();
  BOOST_CHECK(i3.readNonBlocking() == false);
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(o1), 122);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(i3), 10);

  // clear second flag
  i3 = 123;
  i3.setDataValidity(ctk::DataValidity::ok);
  i3.write();
  test.stepApplication();
  o1.read();
  o2.read();
  i3.read();
  BOOST_CHECK(o1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(o2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(o1), 122);
  BOOST_CHECK_EQUAL(o2[0], 10);
  BOOST_CHECK_EQUAL(o2[1], 11);
  BOOST_CHECK(i3.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(i3), 10);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWithFanOut) {
  std::cout << "testWithFanOut" << std::endl;
  TestApplication2 app;
  ctk::TestFacility test;

  auto Ai1 = test.getScalar<int>("A/i1");
  auto Ai2 = test.getArray<int>("A/i2");
  auto Ai3 = test.getScalar<int>("A/i3");
  auto Ao1 = test.getScalar<int>("A/o1");
  auto Ao2 = test.getArray<int>("A/o2");
  auto Bi1 = test.getScalar<int>("B/i1");
  auto Bi2 = test.getArray<int>("B/i2");
  auto Bi3 = test.getScalar<int>("B/i3");
  auto Bo1 = test.getScalar<int>("B/o1");
  auto Bo2 = test.getArray<int>("B/o2");

  test.runApplication();

  app.dumpConnections();

  // test if fault flag propagates to all outputs
  Ai1 = 1;
  Ai1.setDataValidity(ctk::DataValidity::faulty);
  Ai1.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi1.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Ao1), 1);
  BOOST_CHECK_EQUAL(Ao2[0], 0);
  BOOST_CHECK_EQUAL(Ao2[1], 0);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bo1), 1);
  BOOST_CHECK_EQUAL(Bo2[0], 0);
  BOOST_CHECK_EQUAL(Bo2[1], 0);
  BOOST_CHECK(Bi1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bi1), 1);

  // send fault flag on a second variable
  Ai2[0] = 2;
  Ai2[1] = 3;
  Ai2.setDataValidity(ctk::DataValidity::faulty);
  Ai2.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi2.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Ao1), 1);
  BOOST_CHECK_EQUAL(Ao2[0], 2);
  BOOST_CHECK_EQUAL(Ao2[1], 3);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bo1), 1);
  BOOST_CHECK_EQUAL(Bo2[0], 2);
  BOOST_CHECK_EQUAL(Bo2[1], 3);
  BOOST_CHECK(Bi2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(Bi2[0], 2);
  BOOST_CHECK_EQUAL(Bi2[1], 3);

  // clear fault flag on a second variable
  Ai2[0] = 4;
  Ai2[1] = 5;
  Ai2.setDataValidity(ctk::DataValidity::ok);
  Ai2.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi2.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Ao1), 1);
  BOOST_CHECK_EQUAL(Ao2[0], 4);
  BOOST_CHECK_EQUAL(Ao2[1], 5);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::faulty);
  BOOST_CHECK_EQUAL(int(Bo1), 1);
  BOOST_CHECK_EQUAL(Bo2[0], 4);
  BOOST_CHECK_EQUAL(Bo2[1], 5);
  BOOST_CHECK(Bi2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(Bi2[0], 4);
  BOOST_CHECK_EQUAL(Bi2[1], 5);

  // clear fault flag on a first variable
  Ai1 = 6;
  Ai1.setDataValidity(ctk::DataValidity::ok);
  Ai1.write();
  test.stepApplication();
  Ao1.read();
  Ao2.read();
  Bi1.read();
  Bo1.read();
  Bo2.read();
  BOOST_CHECK(Ao1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(Ao2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(Ao1), 6);
  BOOST_CHECK_EQUAL(Ao2[0], 4);
  BOOST_CHECK_EQUAL(Ao2[1], 5);
  BOOST_CHECK(Bo1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK(Bo2.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(Bo1), 6);
  BOOST_CHECK_EQUAL(Bo2[0], 4);
  BOOST_CHECK_EQUAL(Bo2[1], 5);
  BOOST_CHECK(Bi1.dataValidity() == ctk::DataValidity::ok);
  BOOST_CHECK_EQUAL(int(Bi1), 6);
}

/*********************************************************************************************************************/
