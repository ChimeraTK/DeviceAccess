#define BOOST_TEST_MODULE testFanoutConnections

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include "Application.h"
#include "ApplicationModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include <ChimeraTK/ExceptionDummyBackend.h>

namespace ctk = ChimeraTK;

struct TestModule1 : ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPushInput<int> moduleTrigger{this, "moduleTrigger", "", ""};

  ctk::ScalarPollInput<int> i3{this, "i3", "", ""};

  ctk::ScalarOutput<int> moduleOutput{this, "moduleOutput", "", ""};

  void mainLoop() override {
    while(true) {
      moduleTrigger.read();

      i3.readLatest();

      moduleOutput = int(i3);

      writeAll();
    }
  }
};

struct TestApplication1 : ctk::Application {
  TestApplication1() : Application("testApp") {}
  ~TestApplication1() { shutdown(); }

  void defineConnections() {
    findTag(".*").connectTo(cs);
    device.connectTo(cs, cs("deviceTrigger", typeid(int), 1));
  }
  constexpr static char const* dummyCDD1 = "(dummy?map=testDataValidity1.map)";

  TestModule1 m1{this, "m1", ""};
  ctk::DeviceModule device{this, dummyCDD1};

  ctk::ControlSystemModule cs;
};

BOOST_AUTO_TEST_CASE(testConnectConsumingFanout) {
  TestApplication1 theApp;
  ctk::TestFacility testFacility;
  ChimeraTK::Device dummy(TestApplication1::dummyCDD1);

  // write iniial values to the dummy before starting the application
  dummy.open();
  dummy.write("m1/i1/DUMMY_WRITEABLE", 12);
  dummy.write("m1/i3/DUMMY_WRITEABLE", 32);

  testFacility.runApplication();

  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/i1"), 12);
  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/i3"), 32);

  // check that the trigger only affects i1
  dummy.write("m1/i1/DUMMY_WRITEABLE", 13);
  dummy.write("m1/i3/DUMMY_WRITEABLE", 33);

  testFacility.writeScalar<int>("deviceTrigger", 1);
  testFacility.stepApplication();

  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/i1"), 13);
  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/i3"), 32);

  // check that the module trigger updates i3
  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/moduleOutput"), 0);

  dummy.write("m1/i1/DUMMY_WRITEABLE", 14);
  dummy.write("m1/i3/DUMMY_WRITEABLE", 34);

  testFacility.writeScalar<int>("m1/moduleTrigger", 1);
  testFacility.stepApplication();

  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/i1"), 13);
  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/i3"), 34);
  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("m1/moduleOutput"), 34);
}
