/*
 * testBidirectionalVariables.cc
 *
 *  Created on: Jan 21, 2019
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testBidirectionalVariables

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <ChimeraTK/BackendFactory.h>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/

/* Module which converts the input data from inches to centimeters - and the other way round for the return channel.
 * In case of the return channel, the data is rounded downwards to integer inches and sent again forward. */
struct ModuleA : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInputWB<int> var1{this, "var1", "inches", "A length, for some reason rounded to integer"};
    ctk::ScalarOutputPushRB<double> var2{this, "var2", "centimeters", "Same length converted to centimeters"};

    void mainLoop() {
      auto group = readAnyGroup();
      while(true) {
        auto var = group.readAny();
        std::cout << "HIER0 var1=" << var1 << "  var2=" << var2 << std::endl;
        if(var == var2.getId()) {
          var1 = std::floor(var2/2.54);
          std::cout << "HIER1 var1=" << var1 << std::endl;
          var1.write();
        }
        var2 = var1*2.54;
        std::cout << "HIER2 var2=" << var2 << std::endl;
        var2.write();
      }
    }
};

/*********************************************************************************************************************/

/* Module which limits a value to stay below a maximum value. */
struct ModuleB : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInputWB<double> var2{this, "var2", "centimeters", "Some length, confined to a configuratble range"};
    ctk::ScalarPushInput<double> max{this, "max", "centimeters", "Maximum length"};
    ctk::ScalarOutput<double> var3{this, "var3", "centimeters", "The limited length"};

    void mainLoop() {
      auto group = readAnyGroup();
      while(true) {
        auto var = group.readAny();
        bool write = var == var2.getId();
        if(var2 > max) {
          var2 = static_cast<double>(max);
          var2.write();
          write = true;
        }
        if(write) {   // write only if var2 was received or the value was changed due to a reduced limit
          var3 = static_cast<double>(var2);
          var3.write();
        }
      }
    }
};

/*********************************************************************************************************************/

struct TestApplication : public ctk::Application {

    TestApplication() : Application("testSuite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests

    ctk::ControlSystemModule cs;
    ModuleA   a{this,"a", ""};
    ModuleB   b{this,"b", ""};
};

/*********************************************************************************************************************/
/*  */

BOOST_AUTO_TEST_CASE(testNormalOperation) {
    ctk::ExperimentalFeatures::enable();
    std::cout << "*** testNormalOperation" << std::endl;

    TestApplication app;

    //app.a.connectTo(app.cs);
    //app.b.connectTo(app.cs);
    app.cs("var1") >> app.a.var1;
    app.a.var2 >> app.b.var2;
    app.cs("max") >> app.b.max;
    app.cs("var3") >> app.b.var3;
    ctk::TestFacility test;
    app.initialise();
    app.run();
    auto var1 = test.getScalar<int>("var1");
    auto var3 = test.getScalar<double>("var3");
    auto max = test.getScalar<double>("max");

    // set maximum in ModuleC, so that var1=49 is still below maximum but var2=50 is already above and rounding in
    // ModuleB will change the value again
    max = 49.5*2.54;
    max.write();

    // inject value which does not get limited
    var1 = 49;
    var1.write();
    test.stepApplication();
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 49*2.54, 0.001);
    BOOST_CHECK( var1.readNonBlocking() == false );     // nothing was sent through the return channel

    // inject value which gets limited
    var1 = 50;
    var1.write();
    test.stepApplication();
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 49.5*2.54, 0.001);
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 49*2.54, 0.001);
    var1.read();
    BOOST_CHECK_EQUAL(static_cast<int>(var1), 49);

    // change the limit so the current value gets changed
    max = 48.5*2.54;
    max.write();
    test.stepApplication();
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 48.5*2.54, 0.001);
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 48*2.54, 0.001);
    var1.read();
    BOOST_CHECK_EQUAL(static_cast<int>(var1), 48);

/*    // concurrent change of value and limit -> currently deactivated, as not yet working
    var1 = 30;
    max = 25.5*2.54;
    var1.write();
    usleep(1000000);
    max.write();
    test.stepApplication();
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 30*2.54, 0.001);
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 25.5*2.54, 0.001);
    var3.read();
    BOOST_CHECK_CLOSE(static_cast<double>(var3), 25*2.54, 0.001);
    var1.read();
    BOOST_CHECK_EQUAL(static_cast<int>(var1), 25); */

}

/*********************************************************************************************************************/

