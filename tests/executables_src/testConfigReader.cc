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
#include "TestFacility.h"
#include "ConfigReader.h"

namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Module to receive the config values */

struct TestModule : ctk::ApplicationModule {
  TestModule(EntityOwner* owner, const std::string& name, const std::string& description)
  : ctk::ApplicationModule(owner, name, description) {
    try {
      theConfigReader = &appConfig();
    }
    catch(ctk::logic_error&) {
      appConfig_has_thrown = true;
    }
  }

  ctk::ConfigReader* theConfigReader; // just to compare if the correct instance is returned
  bool appConfig_has_thrown{false};

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

  struct Module1 : ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<int16_t> var16{this, "var16", "MV/m", "Desc"};
    ctk::ScalarPushInput<uint16_t> var16u{this, "var16u", "MV/m", "Desc"};
    ctk::ScalarPushInput<int32_t> var32{this, "var32", "MV/m", "Desc"};
    ctk::ScalarPushInput<uint32_t> var32u{this, "var32u", "MV/m", "Desc"};
    ctk::ScalarPushInput<std::string> varString{this, "varString", "MV/m", "Desc"};

    struct SubModule : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<uint32_t> var32u{this, "var32u", "MV/m", "Desc"};
      ctk::ArrayPushInput<int32_t> intArray{this, "intArray", "MV/m", 10, "Desc"};
      ctk::ArrayPushInput<std::string> stringArray{this, "stringArray", "", 8, "Desc"};

      struct SubSubModule : ctk::VariableGroup {
        using ctk::VariableGroup::VariableGroup;
        ctk::ScalarPushInput<int32_t> var32{this, "var32", "MV/m", "Desc"};
        ctk::ScalarPushInput<uint32_t> var32u{this, "var32u", "MV/m", "Desc"};
      } subsubmodule{this, "subsubmodule", ""};
    } submodule{this, "submodule", ""};

  } module1{this, "module1", ""};

  struct Module2 : ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;

    struct AnotherSubModule : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<double> var1{this, "var1", "m", "Desc"};
      ctk::ScalarPushInput<double> var2{this, "var2", "kg", "Desc"};
    } submodule1{this, "submodule1", ""}, submodule2{this, "submodule2", ""};
  } module2{this, "module2", ""};

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
    for(size_t i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(intArray[i], 10 - i);

    BOOST_CHECK_EQUAL(stringArray.getNElements(), 8);
    for(size_t i = 0; i < 8; ++i) BOOST_CHECK_EQUAL(stringArray[i], "Hallo" + std::to_string(i + 1));

    BOOST_CHECK_EQUAL((int16_t)module1.var16, -567);
    BOOST_CHECK_EQUAL((uint16_t)module1.var16u, 678);
    BOOST_CHECK_EQUAL((int32_t)module1.var32, -345678);
    BOOST_CHECK_EQUAL((uint32_t)module1.var32u, 234567);
    BOOST_CHECK_EQUAL((uint32_t)module1.submodule.var32u, 234567);

    BOOST_CHECK_EQUAL(module1.submodule.intArray.getNElements(), 10);
    for(size_t i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(module1.submodule.intArray[i], 10 - i);

    BOOST_CHECK_EQUAL(module1.submodule.stringArray.getNElements(), 8);
    for(size_t i = 0; i < 8; ++i) BOOST_CHECK_EQUAL(module1.submodule.stringArray[i], "Hallo" + std::to_string(i + 1));

    // no further update shall be received
    usleep(1000000); // 1 second
    BOOST_CHECK(!var8.readNonBlocking());
    BOOST_CHECK(!var8u.readNonBlocking());
    BOOST_CHECK(!var16.readNonBlocking());
    BOOST_CHECK(!var16u.readNonBlocking());
    BOOST_CHECK(!var32.readNonBlocking());
    BOOST_CHECK(!var32u.readNonBlocking());
    BOOST_CHECK(!var64.readNonBlocking());
    BOOST_CHECK(!var64u.readNonBlocking());
    BOOST_CHECK(!varFloat.readNonBlocking());
    BOOST_CHECK(!varDouble.readNonBlocking());
    BOOST_CHECK(!varString.readNonBlocking());
    BOOST_CHECK(!intArray.readNonBlocking());

    BOOST_CHECK(!module1.var16.readNonBlocking());
    BOOST_CHECK(!module1.var16u.readNonBlocking());
    BOOST_CHECK(!module1.var32.readNonBlocking());
    BOOST_CHECK(!module1.var32u.readNonBlocking());
    BOOST_CHECK(!module1.submodule.var32u.readNonBlocking());
    BOOST_CHECK(!module1.submodule.intArray.readNonBlocking());
    BOOST_CHECK(!module1.submodule.stringArray.readNonBlocking());

    // inform main thread that we are done
    done = true;
  }
};

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : public ctk::Application {
  TestApplication() : Application("TestApplication") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {} // the setup is done in the tests

  ctk::ConfigReader config{this, "config", "validConfig.xml", {"MyTAG"}};
  TestModule testModule{this, "TestModule", "The test module"};
};

/*********************************************************************************************************************/
/* dummy application with two config readers (to check the exception in ApplicationModule::appConfig()) */

struct TestApplicationTwoConfigs : public ctk::Application {
  TestApplicationTwoConfigs() : Application("TestApplicationTwoConfigs") {}
  ~TestApplicationTwoConfigs() { shutdown(); }

  void defineConnections() {}

  ctk::ConfigReader config{this, "config", "validConfig.xml", {"MyTAG"}};
  ctk::ConfigReader config2{this, "config2", "validConfig.xml"};
  TestModule testModule{this, "TestModule", "The test module"};
};

/*********************************************************************************************************************/
/* dummy application with no config readers (to check the exception in ApplicationModule::appConfig()) */

struct TestApplicationNoConfigs : public ctk::Application {
  TestApplicationNoConfigs() : Application("TestApplicationTwoConfigs") {}
  ~TestApplicationNoConfigs() { shutdown(); }

  void defineConnections() {}

  TestModule testModule{this, "TestModule", "The test module"};
};

/*********************************************************************************************************************/
/* dummy application which directly connects config reader variables to a device */

struct TestApplicationWithDevice : public ctk::Application {
  TestApplicationWithDevice() : Application("TestApplicationWithDevice") {}
  ~TestApplicationWithDevice() { shutdown(); }

  void defineConnections() {
    device.connectTo(config);
    dumpConnections();
  }

  ctk::ConfigReader config{this, "config", "validConfig.xml", {"MyTAG"}};
  ctk::DeviceModule device{this, "(dummy?map=configReaderDevice.map)"};
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an
 * app variable */

BOOST_AUTO_TEST_CASE(testConfigReader) {
  std::cout << "==> testConfigReader" << std::endl;

  TestApplication app;
  BOOST_CHECK(!app.testModule.appConfig_has_thrown);
  BOOST_CHECK(&(app.config) == app.testModule.theConfigReader);

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
  for(size_t i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(arrayValue[i], 10 - i);

  std::vector<std::string> arrayValueString = app.config.get<std::vector<std::string>>("stringArray");
  BOOST_CHECK_EQUAL(arrayValueString.size(), 8);
  for(size_t i = 0; i < 8; ++i) BOOST_CHECK_EQUAL(arrayValueString[i], "Hallo" + std::to_string(i + 1));

  BOOST_CHECK_EQUAL(app.config.get<int16_t>("module1/var16"), -567);
  BOOST_CHECK_EQUAL(app.config.get<uint16_t>("module1/var16u"), 678);
  BOOST_CHECK_EQUAL(app.config.get<int32_t>("module1/var32"), -345678);
  BOOST_CHECK_EQUAL(app.config.get<uint32_t>("module1/var32u"), 234567);
  BOOST_CHECK_EQUAL(app.config.get<uint32_t>("module1/submodule/var32u"), 234567);
  BOOST_CHECK_EQUAL(app.config.get<uint32_t>("module1/submodule/subsubmodule/var32u"), 234568);

  arrayValue = app.config.get<std::vector<int>>("module1/submodule/intArray");
  BOOST_CHECK_EQUAL(arrayValue.size(), 10);
  for(size_t i = 0; i < 10; ++i) BOOST_CHECK_EQUAL(arrayValue[i], 10 - i);

  arrayValueString = app.config.get<std::vector<std::string>>("module1/submodule/stringArray");
  BOOST_CHECK_EQUAL(arrayValueString.size(), 8);
  for(size_t i = 0; i < 8; ++i) BOOST_CHECK_EQUAL(arrayValueString[i], "Hallo" + std::to_string(i + 1));

  //app.config.virtualise().dump();
  app.config.connectTo(app.testModule);

  app.initialise();
  app.run();

  // wait until tests in TestModule::mainLoop() are complete
  while(app.testModule.done == false) usleep(10000);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptions) {
  std::cout << "==> testExceptions" << std::endl;
  {
    TestApplicationTwoConfigs app;
    BOOST_CHECK(app.testModule.appConfig_has_thrown);
  }
  {
    TestApplicationNoConfigs app;
    BOOST_CHECK(app.testModule.appConfig_has_thrown);
  }
  {
    TestApplication app;
    // Test get with types mismatch
    BOOST_CHECK_THROW(app.config.get<uint16_t>("var32u"), ctk::logic_error);

    try {
      app.config.get<uint16_t>("var32u");
    }
    catch(ctk::logic_error& e) {
      std::cout << "Using get with incorrect type. Exception message: " << e.what() << std::endl;
    }

    // Test getting nonexisting varibale
    BOOST_CHECK_THROW(app.config.get<int>("nonexistentVariable"), ctk::logic_error);

    try {
      app.config.get<int>("nonexistentVariable");
    }
    catch(ctk::logic_error& e) {
      std::cout << "Using get with incorrect type. Exception message: " << e.what() << std::endl;
    }

    // Same for arrays
    // Test get with types mismatch
    BOOST_CHECK_THROW(app.config.get<std::vector<float>>("module1/submodule/intArray"), ctk::logic_error);

    try {
      app.config.get<std::vector<float>>("module1/submodule/intArray");
    }
    catch(ctk::logic_error& e) {
      std::cout << "Using get with incorrect type. Exception message: " << e.what() << std::endl;
    }

    // Test getting nonexisting varibale
    BOOST_CHECK_THROW(app.config.get<std::vector<int>>("nonexistentVariable"), ctk::logic_error);

    try {
      app.config.get<std::vector<int>>("nonexistentVariable");
    }
    catch(ctk::logic_error& e) {
      std::cout << "Using get with incorrect type. Exception message: " << e.what() << std::endl;
    }
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDirectWriteToDevice) {
  std::cout << "==> testDirectWriteToDevice" << std::endl;
  TestApplicationWithDevice app;
  ctk::TestFacility test;
  test.runApplication();
  auto var32u = app.device.device.getScalarRegisterAccessor<uint32_t>("var32u");
  auto var16 = app.device.device.getScalarRegisterAccessor<int16_t>("var16");
  auto module1var16 = app.device.device.getScalarRegisterAccessor<int16_t>("module1/var16");
  auto intArray = app.device.device.getOneDRegisterAccessor<int32_t>("intArray");
  var32u.read();
  var16.read();
  module1var16.read();
  intArray.read();
  BOOST_CHECK_EQUAL(var32u, 234567);
  BOOST_CHECK_EQUAL(var16, -567);
  BOOST_CHECK_EQUAL(module1var16, -567);
  BOOST_CHECK_EQUAL(intArray.getNElements(), 10);
  for(size_t i = 0; i < 10; ++i) {
    BOOST_CHECK_EQUAL(intArray[i], 10 - i);
  }
}
