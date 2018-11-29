/*
 * testDirectDeviceToCS.cc
 *
 *  Created on: Jun 22, 2016
 *      Author: Martin Hierholzer
 */

#define BOOST_TEST_MODULE testDirectDeviceToCS

#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/Device.h>
#include "Application.h"
#include "PeriodicTrigger.h"
#include "DeviceModule.h"
#include "ControlSystemModule.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                   \
    {                                                                                                               \
      std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                  \
      while(!(condition)) {                                                                                         \
        bool timeout_reached = (std::chrono::steady_clock::now()-t0) > std::chrono::milliseconds(maxMilliseconds);  \
        BOOST_CHECK( !timeout_reached );                                                                            \
        if(timeout_reached) break;                                                                                  \
        usleep(1000);                                                                                               \
      }                                                                                                             \
    }

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : ctk::Application {
    TestApplication() : Application("testSuite") {}
    ~TestApplication() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {
      dev.connectTo(cs, trigger.tick);
    }

    ctk::PeriodicTrigger trigger{this, "trigger", ""};

    ctk::DeviceModule dev{"(dummy?map=test3.map)"};
    ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/

template<typename T, typename LAMBDA>
void testDirectRegister(ctk::TestFacility &test, ChimeraTK::ScalarRegisterAccessor<T> sender,
                                                 ChimeraTK::ScalarRegisterAccessor<T> receiver,
                                                 LAMBDA trigger, bool testMinMax=true) {

    sender = 42;
    sender.write();
    trigger();
    test.stepApplication();
    receiver.read();
    BOOST_CHECK_EQUAL( receiver, 42 );

    if(std::numeric_limits<T>::is_signed) {
      sender = -120;
      sender.write();
      trigger();
      test.stepApplication();
      receiver.read();
      BOOST_CHECK_EQUAL( receiver, -120 );
    }

    if(testMinMax) {
      sender = std::numeric_limits<T>::max();
      sender.write();
      trigger();
      test.stepApplication();
      receiver.read();
      BOOST_CHECK_EQUAL( receiver, std::numeric_limits<T>::max() );

      sender = std::numeric_limits<T>::min();
      sender.write();
      trigger();
      test.stepApplication();
      receiver.read();
      BOOST_CHECK_EQUAL( receiver, std::numeric_limits<T>::min() );

      sender = std::numeric_limits<T>::epsilon();
      sender.write();
      trigger();
      test.stepApplication();
      receiver.read();
      BOOST_CHECK_EQUAL( receiver, std::numeric_limits<T>::epsilon() );
    }

}

/*********************************************************************************************************************/
/* test connectTo */

BOOST_AUTO_TEST_CASE( testConnectTo ) {
  std::cout << "testConnectTo" << std::endl;

  ctk::Device dev;
  dev.open("(dummy?map=test3.map)");

  TestApplication app;

  ctk::TestFacility test;
  auto devActuator = dev.getScalarRegisterAccessor<int32_t>("/MyModule/actuator");
  auto devReadback = dev.getScalarRegisterAccessor<int32_t>("/MyModule/readBack");
  auto devint32 = dev.getScalarRegisterAccessor<int32_t>("/Integers/signed32");
  auto devuint32 = dev.getScalarRegisterAccessor<uint32_t>("/Integers/unsigned32");
  auto devint16 = dev.getScalarRegisterAccessor<int16_t>("/Integers/signed16");
  auto devuint16 = dev.getScalarRegisterAccessor<uint16_t>("/Integers/unsigned16");
  auto devint8 = dev.getScalarRegisterAccessor<int8_t>("/Integers/signed8");
  auto devuint8 = dev.getScalarRegisterAccessor<uint8_t>("/Integers/unsigned8");
  auto devfloat = dev.getScalarRegisterAccessor<double>("/FixedPoint/value");
  auto csActuator = test.getScalar<int32_t>("/MyModule/actuator");
  auto csReadback = test.getScalar<int32_t>("/MyModule/readBack");
  auto csint32 = test.getScalar<int32_t>("/Integers/signed32");
  auto csuint32 = test.getScalar<uint32_t>("/Integers/unsigned32");
  auto csint16 = test.getScalar<int16_t>("/Integers/signed16");
  auto csuint16 = test.getScalar<uint16_t>("/Integers/unsigned16");
  auto csint8 = test.getScalar<int8_t>("/Integers/signed8");
  auto csuint8 = test.getScalar<uint8_t>("/Integers/unsigned8");
  auto csfloat = test.getScalar<double>("/FixedPoint/value");
  test.runApplication();

  testDirectRegister(test, csActuator, devActuator, []{});
  testDirectRegister(test, devReadback, csReadback, [&]{app.trigger.sendTrigger();});
  testDirectRegister(test, csint32, devint32, []{});
  testDirectRegister(test, csuint32, devuint32, []{});
  testDirectRegister(test, csint16, devint16, []{});
  testDirectRegister(test, csuint16, devuint16, []{});
  testDirectRegister(test, csint8, devint8, []{});
  testDirectRegister(test, csuint8, devuint8, []{});
  testDirectRegister(test, csfloat, devfloat, []{}, false);

}
