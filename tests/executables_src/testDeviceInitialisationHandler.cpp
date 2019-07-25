//#include <future>

#define BOOST_TEST_MODULE testDeviceInitialisationHandler

//#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/test_case_template.hpp>

#include "Application.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
//#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "ExceptionDevice.h"

#include <ChimeraTK/Device.h>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

void initialiseReg1(ctk::DeviceModule * dev){
    dev->device.write<int32_t>("/REG1",42);
}

void initialiseReg2(ctk::DeviceModule * dev){
    // the initialisation of reg 2 must happen after the initialisation of reg1
    dev->device.write<int32_t>("/REG2", dev->device.read<int32_t>("/REG1")+5);
}

void initialiseReg3(ctk::DeviceModule * dev){
    // the initialisation of reg 3 must happen after the initialisation of reg2
    dev->device.write<int32_t>("/REG3", dev->device.read<int32_t>("/REG2")+5);
}

/* dummy application */
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests
  ctk::ControlSystemModule cs;
  ctk::DeviceModule dev{this, "(ExceptionDummy?map=test.map)",&initialiseReg1};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBasicInitialisation) {
  std::cout << "testBasicInitialisation" << std::endl;
  TestApplication app;

  app.dev.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  ctk::Device dummy;
  dummy.open("(ExceptionDummy?map=test.map)");
  auto reg1 = dummy.getScalarRegisterAccessor<int32_t>("/REG1");
  reg1.read();

  // ********************************************************
  // REQUIRED TEST 1: After opening the device is initialised
  // ********************************************************
  BOOST_CHECK_EQUAL(reg1,42);

  reg1=0;
  reg1.write();

  // check that accessing an exception triggers a reconnection with re-initialisation
  auto dummyBackend = boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend("(ExceptionDummy?map=test.map)"));
  dummyBackend->throwExceptionWrite=true;

  // FIXME: Due to a bug it is /REG2/REG2 instead of just /REG2. This will fails once the bug has been solved.
  auto reg2_cs = test.getScalar<int32_t>("/REG2/REG2");
  reg2_cs=19;
  reg2_cs.write();
  test.stepApplication();

  auto reg2 = dummy.getScalarRegisterAccessor<int32_t>("/REG2");
  reg2.read();

  BOOST_CHECK_EQUAL(reg2,0);
  BOOST_CHECK_EQUAL(reg1,0);
  dummyBackend->throwExceptionWrite=false; // now the device should work again and be re-initialised

  reg2_cs=20;
  reg2_cs.write();
  test.stepApplication();

  reg2.read();
  BOOST_CHECK_EQUAL(reg2,20);

  // ****************************************************************
  // REQUIRED TEST 2: After an exception the device is re-initialised
  // ****************************************************************
  reg1.read();
  BOOST_CHECK_EQUAL(reg1, 42);


}

