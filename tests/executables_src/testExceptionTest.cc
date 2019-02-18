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

/* Module which limits a value to stay below a maximum value. */
struct ModuleB : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<double> var1{this, "var1", "centimeters", "Some length, confined to a configuratble range"};
  ctk::ScalarOutput<double> var2{this, "var2", "centimeters", "The limited length"};

  void mainLoop() {
    auto group = readAnyGroup();
    while(true) {
      auto var = group.readAny();
      if(var == var2.getId()) {
        var1 = std::floor(var2 / 2.54);
        var1.write();
      }
      var2 = var1 * 2.54;
      var2.write();
    }
  }
};

/*********************************************************************************************************************/

/* dummy application */

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  using Application::makeConnections; // we call makeConnections() manually in the tests to catch exceptions etc.

  void defineConnections() {
    // testableMode = true;
    debugTestableMode();

    dumpConnections();
  } // the setup is done in the tests

  ctk::DeviceModule dev{this, "(ExceptionDummy?map=DemoDummy.map)"};
  // ModuleB   b;
  ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testThinkOfAName) {
  std::cout << "testThinkOfAName" << std::endl;

  TestApplication app;

  // app.b = {&app,"b", ""};

  /*app.b.var2 >> app.dev("probeSignal");

  app.b.connectTo(app.cs);

  app.cs("probeSignal") >> app.dev("probeSignal");

  app.dev.connectTo(app.cs);
  //app.cs >> app.b.var1;*/
  // app.cs("probeSignal") >> app.dev("probeSignal");
  app.dev.connectTo(app.cs);
  std::cout << "CONNECTED" << std::endl;
  ctk::TestFacility test;
  app.initialise();
  app.run();

  std::cout << "RUNNING" << std::endl;

  /*auto var1 = test.getScalar<uint32_t>("probeSignal");
  var1 = 10;
  var1.write();
  test.stepApplication();*/
  // std::cout<<"STEP"<<std::endl;
  // app.dev.reportException("exception");

  boost::shared_ptr<ExceptionDummy> backend = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("(ExceptionDummy?map=DemoDummy.map)"));
  backend->close();
  std::cout << "backend closed" << std::endl;
  backend->throwException = true;
  try {
    backend->open();
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::runtime_error&) {
  }
  std::cout << "report an exception" << std::endl;
  app.dev.reportException("exception");
  sleep(5);
  backend->throwException = false;
  std::cout << "THE END" << std::endl;

  /*auto var2 = test.getScalar<double>("var2");
  var2.read();
  test.stepApplication();
  std::cout<<var2<<std::endl;*/
}
