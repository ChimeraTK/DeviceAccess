#define BOOST_TEST_MODULE testTestFaciliy
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "ApplicationCore.h"
#include "TestFacility.h"
namespace ctk=ChimeraTK;


struct MyModule : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInput<double> input{this, "input", "", ""};
    ctk::ScalarOutput<double> output{this, "output", "", ""};

    void mainLoop() override{
        std::cout << "startin main loop" << std::endl;
        output = 2*double(input);
        output.write();

        while(true){
            input.read();
            output = 3*double(input);
            output.write();
        }
    }
};

/**********************************************************************************************************************/

struct TestApp : public ctk::Application {
    TestApp() : Application("TestApp") {}
    ~TestApp() override { shutdown(); }

    ctk::ControlSystemModule cs;
    MyModule myModule{this, "MyModule", ""};

    void defineConnections() override{
        myModule.connectTo(cs);
    }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testSumLimiter){
    TestApp theTestApp;
    ChimeraTK::TestFacility testFacility;

    testFacility.writeScalar<double>("/input",25.);
    sleep(2);
    testFacility.runApplication();

    // at this point all main loops should have started and inputs waiting in read()

    BOOST_CHECK_CLOSE(testFacility.readScalar<double>("/output"), 50., 0.001);

    testFacility.writeScalar<double>("/input",30.);
    std::cout << "about to step" << std::endl;
    // however, the main loop only starts in the first step.
    testFacility.stepApplication();
    std::cout << "step finished" << std::endl;

    BOOST_CHECK_CLOSE(testFacility.readScalar<double>("/output"), 90., 0.001);
}

/**********************************************************************************************************************/

