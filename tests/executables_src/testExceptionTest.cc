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
          var1 = std::floor(var2/2.54);
          var1.write();
        }
        var2 = var1*2.54;
        var2.write();
      }
    }
};

/*********************************************************************************************************************/

/* dummy application */


struct TestApplication : public ctk::Application {
    TestApplication() : Application("testSuite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    
    void defineConnections() {dumpConnections(); }             // the setup is done in the tests
    
    ctk::DeviceModule dev{this,"Exception"};
    //ModuleB   b;
    ctk::ControlSystemModule cs;
    
};

/*********************************************************************************************************************/


BOOST_AUTO_TEST_CASE(testThinkOfAName) {
  std::cout << "testThinkOfAName" << std::endl;

  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("dummy2.dmap");
  
  
  TestApplication app;

  //app.b = {&app,"b", ""};
  
  /*app.b.var2 >> app.dev("probeSignal");
  
  app.b.connectTo(app.cs);
  
  app.cs("probeSignal") >> app.dev("probeSignal");*/
  
  app.dev.connectTo(app.cs);
  //app.cs >> app.b.var1;
  
  ctk::TestFacility test;
  app.initialise();
  app.run();

  auto var1 = test.getScalar<uint32_t>("probeSignal/probeSignal");
  var1 = 10;
  var1.write();
  test.stepApplication();
  
  boost::shared_ptr< ExceptionDummy > backend = boost::dynamic_pointer_cast<ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("Exception") );
  backend->close();
  backend->throwException = true;    
  //backend->open();
  app.dev.reportException("exception");

  
  /*auto var2 = test.getScalar<double>("var2");
  var2.read();
  test.stepApplication();
  std::cout<<var2<<std::endl;*/
}
