/*
 * testConfigReader.cc
 *
 *  Created on: Oct 12, 2017
 *      Author: Martin Hierholzer
 */

#define BOOST_TEST_MODULE testConfigReader

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "ApplicationCore.h"
#include "ConfigReader.h"

namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Module to receive the config values */

struct TestModule : ctk::ApplicationModule { using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int8_t> var8{this, "var8", "MV/m", "Desc"};
  ctk::ScalarPushInput<uint8_t> var8u{this, "var8u", "MV/m", "Desc"};
  ctk::ScalarPushInput<int16_t> var16{this, "var16", "MV/m", "Desc"};
  ctk::ScalarPushInput<uint16_t> var16u{this, "var16u", "MV/m", "Desc"};
  ctk::ScalarPushInput<int32_t> var32{this, "var32", "MV/m", "Desc"};
  ctk::ScalarPushInput<uint32_t> var32u{this, "var32u", "MV/m", "Desc"};
  ctk::ScalarPushInput<int64_t> var64{this, "var64", "MV/m", "Desc"};
  ctk::ScalarPushInput<uint64_t> var64u{this, "var64u", "MV/m", "Desc"};
  ctk::ScalarPushInput<float> varFloat{this, "varFloat", "MV/m", "Desc"};
  ctk::ScalarPushInput<double> varDouble{this, "varDouble", "MV/m", "Desc"};
  ctk::ScalarPushInput<std::string> varString{this, "varString", "MV/m", "Desc"};
  ctk::ScalarPushInput<int32_t> varAnotherInt{this, "varAnotherInt", "MV/m", "Desc"};
  ctk::ArrayPushInput<int32_t> intArray{this, "intArray", "MV/m", 10, "Desc"};
  ctk::ArrayPushInput<std::string> stringArray{this, "stringArray", "", 8, "Desc"};

  std::atomic<bool> done{false};

  void mainLoop() {

    // values should be available right away
    BOOST_CHECK_EQUAL((int8_t)var8, -123);
    BOOST_CHECK_EQUAL((uint8_t)var8u, 34);
    BOOST_CHECK_EQUAL((int16_t)var16, -567);
    BOOST_CHECK_EQUAL((uint16_t)var16u, 678);
    BOOST_CHECK_EQUAL((int32_t)var32, -345678);
    BOOST_CHECK_EQUAL((uint32_t)var32u, 234567);
    BOOST_CHECK_EQUAL((int64_t)var64, -2345678901234567890);
    BOOST_CHECK_EQUAL((uint64_t)var64u, 12345678901234567890U);
    BOOST_CHECK_CLOSE((float)varFloat, 3.1415, 0.000001);
    BOOST_CHECK_CLOSE((double)varDouble, -2.8, 0.000001);
    BOOST_CHECK_EQUAL((std::string)varString, "My dear mister singing club!");

    BOOST_CHECK_EQUAL(intArray.getNElements(), 10);
    for(size_t i=0; i<10; ++i) BOOST_CHECK_EQUAL(intArray[i], 10-i);

    BOOST_CHECK_EQUAL(stringArray.getNElements(), 8);
    for(size_t i=0; i<8; ++i) BOOST_CHECK_EQUAL(stringArray[i], "Hallo"+std::to_string(i+1));

    // no further update shall be received
    usleep(1000000);   // 1 second
    BOOST_CHECK( !var8.readNonBlocking() );
    BOOST_CHECK( !var8u.readNonBlocking() );
    BOOST_CHECK( !var16.readNonBlocking() );
    BOOST_CHECK( !var16u.readNonBlocking() );
    BOOST_CHECK( !var32.readNonBlocking() );
    BOOST_CHECK( !var32u.readNonBlocking() );
    BOOST_CHECK( !var64.readNonBlocking() );
    BOOST_CHECK( !var64u.readNonBlocking() );
    BOOST_CHECK( !varFloat.readNonBlocking() );
    BOOST_CHECK( !varDouble.readNonBlocking() );
    BOOST_CHECK( !varString.readNonBlocking() );
    BOOST_CHECK( !intArray.readNonBlocking() );

    // inform main thread that we are done
    done = true;
  }

};

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : public ctk::Application {
    TestApplication() : Application("testSuite") {}
    ~TestApplication() { shutdown(); }

    void defineConnections() {}             // the setup is done in the tests

    ctk::ConfigReader config{this, "config", "validConfig.xml", {"MyTAG"}};
    TestModule testModule{this, "TestModule", "The test module"};
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an app variable */

BOOST_AUTO_TEST_CASE( testConfigReader ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testConfigReader" << std::endl;

  TestApplication app;

  // check if values are already accessible
  BOOST_CHECK_EQUAL(app.config.get<int8_t>("var8"), -123);
  BOOST_CHECK_EQUAL(app.config.get<uint8_t>("var8u"), 34);
  BOOST_CHECK_EQUAL(app.config.get<int16_t>("var16"), -567);
  BOOST_CHECK_EQUAL(app.config.get<uint16_t>("var16u"), 678);
  BOOST_CHECK_EQUAL(app.config.get<int32_t>("var32"), -345678);
  BOOST_CHECK_EQUAL(app.config.get<uint32_t>("var32u"), 234567);
  BOOST_CHECK_EQUAL(app.config.get<int64_t>("var64"), -2345678901234567890);
  BOOST_CHECK_EQUAL(app.config.get<uint64_t>("var64u"), 12345678901234567890U);
  BOOST_CHECK_CLOSE(app.config.get<float>("varFloat"), 3.1415, 0.000001);
  BOOST_CHECK_CLOSE(app.config.get<double>("varDouble"), -2.8, 0.000001);
  BOOST_CHECK_EQUAL(app.config.get<std::string>("varString"), "My dear mister singing club!");

  std::vector<int> arrayValue = app.config.get<std::vector<int>>("intArray");
  BOOST_CHECK_EQUAL(arrayValue.size(), 10);
  for(size_t i=0; i<10; ++i) BOOST_CHECK_EQUAL(arrayValue[i], 10-i);

  std::vector<std::string> arrayValueString = app.config.get<std::vector<std::string>>("stringArray");
  BOOST_CHECK_EQUAL(arrayValueString.size(), 8);
  for(size_t i=0; i<8; ++i) BOOST_CHECK_EQUAL(arrayValueString[i], "Hallo"+std::to_string(i+1));

  app.config.connectTo(app.testModule);

  app.initialise();
  app.run();

  // wait until tests in TestModule::mainLoop() are complete
  while(app.testModule.done == false) usleep(10000);

}

