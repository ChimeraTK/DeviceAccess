#define BOOST_TEST_MODULE testInitialValues

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <cstring>
#include <future>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/DummyRegisterAccessor.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "check_timeout.h"

#include <future>
#include <functional>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

struct PollModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPollInput<int> pollInput{this, "REG1", "", "", {"DEVICE"}};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct PollDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  PollDummyApplication() : Application("DummyApplication") {}
  ~PollDummyApplication() { shutdown(); }

  PollModule pollModule{this, "", ""};
  ChimeraTK::ControlSystemModule cs;
  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    findTag("CS").connectTo(cs);
    findTag("DEVICE").connectTo(device);
  }
};

struct PollTypeInitialValueEceptionDummy {
  PollTypeInitialValueEceptionDummy()
    : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
                      ChimeraTK::BackendFactory::getInstance().createBackend(PollDummyApplication::ExceptionDummyCDD1))) {}
  ~PollTypeInitialValueEceptionDummy() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  PollDummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPollInput<int>& pollVariable{application.pollModule.pollInput};
};
/**
  *  Test Initial Values - Inputs of `ApplicationModule`s
  *  \anchor testInitialValuesInputsOfApplicationCore \ref InitialValuesInputsOfApplicationCore_D_8 "D.8"
  */
BOOST_AUTO_TEST_SUITE(testInitialValuesInputsOfApplicationCore_D_8)

/**
  *  For device variables the ExeptionHandlingDecorator freezes the variable until the device is available
  * \anchor testInitialValue_D_8_b_i \ref initialValue_D_8_b_i
  */
BOOST_AUTO_TEST_CASE(testPollInitValueAtDevice8bi) {
  std::cout << "===   testPollInitValueAtDevice8bi   === " << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  { // Here the time is stopped until you reach the mainloop.
    PollTypeInitialValueEceptionDummy dummyToStopTimeUntilOpen;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeUntilOpen.application.run();
    dummyToStopTimeUntilOpen.application.pollModule.p.get_future().wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    end = std::chrono::steady_clock::now();
  }
  { // waiting 2 x the time stoped above, in the assumption that it is then freezed,
    // as it is described in the spec.
    PollTypeInitialValueEceptionDummy d;
    d.deviceBackend->throwExceptionOpen = true;
    BOOST_CHECK_THROW(d.deviceBackend->open(),std::exception);
    d.application.run();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
      (end-start).count();
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2*elapsed_milliseconds));
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.pollVariable.getVersionNumber() == ctk::VersionNumber( std::nullptr_t() ));
    d.deviceBackend->throwExceptionOpen = false;
    d.exceptionDummyRegister.replace(d.application.device.device.getScalarRegisterAccessor<int>("/REG1"));//
    d.application.pollModule.p.get_future().wait();
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.pollVariable.getVersionNumber() != ctk::VersionNumber( std::nullptr_t() ));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct PushModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
//  ChimeraTK::ScalarPushInput<int> pushInput{this, "REG1", "", "", {"DEVICE"}};
  struct : ChimeraTK::VariableGroup {
    using ChimeraTK::VariableGroup::VariableGroup;
    ChimeraTK::ScalarPushInput<int> pushInput{this, "PUSH_READ", "", ""};
  } reg1{this, "REG1", ""};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct PushDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  PushDummyApplication() : Application("DummyApplication") {}
  ~PushDummyApplication() { shutdown(); }

  PushModule pushModule{this, "", ""};
  ChimeraTK::ControlSystemModule cs;
  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    findTag("CS").connectTo(cs);
    findTag("DEVICE").connectTo(device);
    auto push_input = device("REG1/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::push);
    push_input >> pushModule.reg1.pushInput;
  }
};

struct PushTypeInitialValueEceptionDummy {
  PushTypeInitialValueEceptionDummy()
    : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
                      ChimeraTK::BackendFactory::getInstance().createBackend(PushDummyApplication::ExceptionDummyCDD1))) {}
  ~PushTypeInitialValueEceptionDummy() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  PushDummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPushInput<int>& pushVariable{application.pushModule.reg1.pushInput};
};

BOOST_AUTO_TEST_CASE(testPushInitValueAtDevice8bi) {
  std::cout << "===   testPushInitValueAtDevice8bi   === " << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  {
    PushTypeInitialValueEceptionDummy dummyToStopTimeUntilOpen;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeUntilOpen.application.run();
    dummyToStopTimeUntilOpen.application.pushModule.p.get_future().wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    end = std::chrono::steady_clock::now();
  }
  {
    PushTypeInitialValueEceptionDummy d;
    d.deviceBackend->throwExceptionOpen = true;
    BOOST_CHECK_THROW(d.deviceBackend->open(),std::exception);
    d.application.run();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
      (end-start).count();
    BOOST_CHECK(d.application.pushModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2*elapsed_milliseconds));
    BOOST_CHECK(d.application.pushModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.pushVariable.getVersionNumber() == ctk::VersionNumber( std::nullptr_t() ));
    d.deviceBackend->throwExceptionOpen = false;
    d.application.pushModule.p.get_future().wait();
    BOOST_CHECK(d.application.pushModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.pushVariable.getVersionNumber() != ctk::VersionNumber( std::nullptr_t() ));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct PollInputModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPollInput<int> pollInput{this, "REG1", "", "", {"DEVICE"}};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct ScalarOutputModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarOutput<int> output{this, "REG1", "", "", {"DEVICE"}};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct PollProcessArryDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  PollProcessArryDummyApplication() : Application("DummyApplication") {}
  ~PollProcessArryDummyApplication() { shutdown(); }
  ChimeraTK::ControlSystemModule cs;

  PollInputModule pollInputModule{this, "", ""};
  ScalarOutputModule scalarOutputModule{this, "", ""};
  void defineConnections() override {
    findTag("CS").connectTo(cs);
    scalarOutputModule.connectTo(pollInputModule);
  }
};

struct PollProcessArrayTypeInitialValueEceptionDummy {
  PollProcessArrayTypeInitialValueEceptionDummy()
    : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
                      ChimeraTK::BackendFactory::getInstance().createBackend(PollDummyApplication::ExceptionDummyCDD1))) {}
  ~PollProcessArrayTypeInitialValueEceptionDummy() {
    deviceBackend->throwExceptionRead = false;
    deviceBackend->throwExceptionOpen = false;}

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  PollProcessArryDummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPollInput<int>& pollInputVariable{application.pollInputModule.pollInput};
  ChimeraTK::ScalarOutput<int>& pollOutputVariable{application.scalarOutputModule.output};
};
/**
  *  ProcessArray freeze in their implementation until the initial value is received
  * \anchor testInitialValue_D_8_b_ii \ref initialValue_D_8_b_ii
  */
BOOST_AUTO_TEST_CASE(testPollProcessArrayInitValueAtDevice8bii) {
  std::cout << "===   testPollProcessArrayInitValueAtDevice8bii   === " << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  {
    PollProcessArrayTypeInitialValueEceptionDummy dummyToStopTimeUntilOpen;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeUntilOpen.application.run();
    dummyToStopTimeUntilOpen.pollOutputVariable.write();
    dummyToStopTimeUntilOpen.application.pollInputModule.p.get_future().wait();
    end = std::chrono::steady_clock::now();
  }
  {
    PollProcessArrayTypeInitialValueEceptionDummy d;
    d.application.run();
    BOOST_CHECK(d.application.pollInputModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(end - start);
    BOOST_CHECK(d.application.pollInputModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.pollInputVariable.getVersionNumber() == ctk::VersionNumber( std::nullptr_t() ));
    d.pollOutputVariable.write();
    d.application.pollInputModule.p.get_future().wait();
    BOOST_CHECK(d.application.pollInputModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.pollInputVariable.getVersionNumber() != ctk::VersionNumber( std::nullptr_t() ));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConstantPollModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPollInput<int> constantPollInput{this, "REG1", "", "", {"DEVICE"}};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct ConstantPollDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  ConstantPollDummyApplication() : Application("DummyApplication") {}
  ~ConstantPollDummyApplication() { shutdown(); }

  ConstantPollModule constantPollModule{this, "", ""};
  ChimeraTK::ControlSystemModule cs;
  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    findTag("CS").connectTo(cs);
  }
};

struct ConstantPollTypeInitialValueEceptionDummy {
  ConstantPollTypeInitialValueEceptionDummy()
    : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
                      ChimeraTK::BackendFactory::getInstance().createBackend(ConstantPollDummyApplication::ExceptionDummyCDD1))) {}
  ~ConstantPollTypeInitialValueEceptionDummy() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  ConstantPollDummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPollInput<int>& constantPollInput{application.constantPollModule.constantPollInput};
};
/**
  * Constants can be read exactly once in case of `AccessMode::wait_for_new_data`, so the initial value can be received.
  * \anchor testInitialValue_D_8_b_iii \ref initialValue_D_8_b_iii
  */
BOOST_AUTO_TEST_CASE(testConstantPollInitValueAtDevice8biii) {
  std::cout << "===   testConstantPollInitValueAtDevice8biii   === " << std::endl;
  ConstantPollTypeInitialValueEceptionDummy d;

  d.application.run();
  BOOST_CHECK(d.constantPollInput.getVersionNumber() == ctk::VersionNumber( std::nullptr_t() ));
  BOOST_CHECK(d.application.constantPollModule.enteredTheMainLoop == false);
  d.application.constantPollModule.p.get_future().wait();
  BOOST_CHECK(d.application.constantPollModule.enteredTheMainLoop == true);
  BOOST_CHECK(d.constantPollInput.getVersionNumber() != ctk::VersionNumber( std::nullptr_t() ));

}
BOOST_AUTO_TEST_SUITE_END()
