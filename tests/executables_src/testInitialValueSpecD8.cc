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
  ChimeraTK::ScalarPollInput<int> pollInput{this, "REG1", "", ""};
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
  ~PollDummyApplication() override { shutdown(); }

  PollModule pollModule{this, "PollModule", ""};

  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override { pollModule.connectTo(device); }
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
    BOOST_CHECK_THROW(d.deviceBackend->open(), std::exception);
    d.application.run();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * elapsed_milliseconds));
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.pollVariable.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.deviceBackend->throwExceptionOpen = false;
    d.exceptionDummyRegister.replace(d.application.device.device.getScalarRegisterAccessor<int>("/REG1")); //
    d.application.pollModule.p.get_future().wait();
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.pollVariable.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct PushModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
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
  ~PushDummyApplication() override { shutdown(); }

  PushModule pushModule{this, "PushModule", ""};

  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
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
    BOOST_CHECK_THROW(d.deviceBackend->open(), std::exception);
    d.application.run();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    BOOST_CHECK(d.application.pushModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * elapsed_milliseconds));
    BOOST_CHECK(d.application.pushModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.pushVariable.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.deviceBackend->throwExceptionOpen = false;
    d.application.pushModule.p.get_future().wait();
    BOOST_CHECK(d.application.pushModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.pushVariable.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ScalarOutputModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarOutput<int> output{this, "REG1", "", ""};
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
  ~PollProcessArryDummyApplication() override { shutdown(); }

  PollModule pollModule{this, "PollModule", ""};
  ScalarOutputModule scalarOutputModule{this, "ScalarOutputModule", ""};
  void defineConnections() override { scalarOutputModule.connectTo(pollModule); }
};

struct PollProcessArrayTypeInitialValueEceptionDummy {
  PollProcessArrayTypeInitialValueEceptionDummy()
  : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(PollDummyApplication::ExceptionDummyCDD1))) {}
  ~PollProcessArrayTypeInitialValueEceptionDummy() {
    deviceBackend->throwExceptionRead = false;
    deviceBackend->throwExceptionOpen = false;
  }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  PollProcessArryDummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPollInput<int>& pollInputVariable{application.pollModule.pollInput};
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
    dummyToStopTimeUntilOpen.application.pollModule.p.get_future().wait();
    end = std::chrono::steady_clock::now();
  }
  {
    PollProcessArrayTypeInitialValueEceptionDummy d;
    d.application.run();
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(end - start);
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.pollInputVariable.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.pollOutputVariable.write();
    d.application.pollModule.p.get_future().wait();
    BOOST_CHECK(d.application.pollModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.pollInputVariable.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConstantPollModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPollInput<int> constantPollInput{this, "REG1", "", ""};
  std::promise<void> p;
  void mainLoop() override { p.set_value(); }
};

struct ConstantPushModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPushInput<int> constantPushInput{this, "REG2", "", "", {"DEVICE"}};
  std::promise<void> p;
  void mainLoop() override { p.set_value(); }
};

struct ConstantPollDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  ConstantPollDummyApplication() : Application("DummyApplication") {}
  ~ConstantPollDummyApplication() override { shutdown(); }

  ConstantPollModule constantPollModule{this, "constantPollModule", ""};

  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {}
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
};
/**
  * Constants can be read exactly once in case of `AccessMode::wait_for_new_data`, so the initial value can be received.
  * \anchor testInitialValue_D_8_b_iii \ref initialValue_D_8_b_iii
  */
BOOST_AUTO_TEST_CASE(testConstantPollInitValueAtDevice8biii) {
  std::cout << "===   testConstantPollInitValueAtDevice8biii   === " << std::endl;
  ConstantPollTypeInitialValueEceptionDummy d;

  BOOST_CHECK(
      d.application.constantPollModule.constantPollInput.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));

  d.application.run();

  d.application.constantPollModule.p.get_future().wait();

  BOOST_CHECK(
      d.application.constantPollModule.constantPollInput.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct PushModuleD9_1 : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
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
struct PushModuleD9_2 : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  struct : ChimeraTK::VariableGroup {
    using ChimeraTK::VariableGroup::VariableGroup;
    ChimeraTK::ScalarPushInput<int> pushInput{this, "PUSH_READ", "", ""};
  } reg1{this, "REG2", ""};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct PushD9DummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  PushD9DummyApplication() : Application("DummyApplication") {}
  ~PushD9DummyApplication() override { shutdown(); }

  PushModuleD9_1 pushModuleD9_1{this, "PushModule1", ""};
  PushModuleD9_2 pushModuleD9_2{this, "PushModule2", ""};

  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    auto push_input1 = device("REG1/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::push);
    auto push_input2 = device("REG2/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::push);
    push_input1 >> pushModuleD9_1.reg1.pushInput;
    push_input2 >> pushModuleD9_2.reg1.pushInput;
  }
};

struct D9InitialValueEceptionDummy {
  D9InitialValueEceptionDummy()
  : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(PushDummyApplication::ExceptionDummyCDD1))) {}
  ~D9InitialValueEceptionDummy() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  PushD9DummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPushInput<int>& pushVariable1{application.pushModuleD9_1.reg1.pushInput};
  ChimeraTK::ScalarPushInput<int>& pushVariable2{application.pushModuleD9_2.reg1.pushInput};
};

/**
  *  D 9 b for ThreaddedFanOut
  * \anchor testInitialValueThreaddedFanOut_D_9_b \ref initialValueThreaddedFanOut_D_9_b
  */
BOOST_AUTO_TEST_CASE(testPushInitValueAtDeviceD9) {
  std::cout << "===   testPushInitValueAtDeviceD9   === " << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  {
    D9InitialValueEceptionDummy dummyToStopTimeUntilOpen;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeUntilOpen.application.run();
    dummyToStopTimeUntilOpen.application.pushModuleD9_1.p.get_future().wait();
    dummyToStopTimeUntilOpen.application.pushModuleD9_2.p.get_future().wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    end = std::chrono::steady_clock::now();
  }
  {
    D9InitialValueEceptionDummy d;
    d.deviceBackend->throwExceptionOpen = true;
    BOOST_CHECK_THROW(d.deviceBackend->open(), std::exception);
    d.application.run();
    BOOST_CHECK(d.application.pushModuleD9_1.enteredTheMainLoop == false);
    std::this_thread::sleep_for(2 * (end - start));
    BOOST_CHECK(d.application.pushModuleD9_1.enteredTheMainLoop == false);
    BOOST_CHECK(d.pushVariable1.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.deviceBackend->throwExceptionOpen = false;
    d.application.pushModuleD9_1.p.get_future().wait();
    BOOST_CHECK(d.application.pushModuleD9_1.enteredTheMainLoop == true);
    BOOST_CHECK(d.pushVariable1.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TriggerModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  struct : ChimeraTK::VariableGroup {
    using ChimeraTK::VariableGroup::VariableGroup;
    ChimeraTK::ScalarOutput<int> trigger{this, "PUSH_OUT", "", ""};
  } trigger{this, "TRIG1", ""};
  std::promise<void> p;
  std::atomic_bool enteredTheMainLoop{false};
  void mainLoop() override {
    enteredTheMainLoop = true;
    p.set_value();
  }
};

struct TriggerFanOutD9DummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  TriggerFanOutD9DummyApplication() : Application("DummyApplication") {}
  ~TriggerFanOutD9DummyApplication() override { shutdown(); }

  PushModuleD9_1 pushModuleD9_1{this, "PushModule1", ""};
  PushModuleD9_2 pushModuleD9_2{this, "PushModule2", ""};
  TriggerModule triggerModule{this, "TriggerModule", ""};

  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    auto pollInput1 = device("REG1/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::poll);
    auto pollInput2 = device("REG2/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::poll);
    auto trigger = triggerModule["TRIG1"]("PUSH_OUT");
    pollInput1[trigger] >> pushModuleD9_1.reg1.pushInput;
    pollInput2[trigger] >> pushModuleD9_2.reg1.pushInput;
  }
};

struct TriggerFanOutInitialValueEceptionDummy {
  TriggerFanOutInitialValueEceptionDummy()
  : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(PushDummyApplication::ExceptionDummyCDD1))) {}
  ~TriggerFanOutInitialValueEceptionDummy() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  TriggerFanOutD9DummyApplication application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
  ChimeraTK::ScalarPushInput<int>& pushVariable1{application.pushModuleD9_1.reg1.pushInput};
  ChimeraTK::ScalarPushInput<int>& pushVariable2{application.pushModuleD9_2.reg1.pushInput};
};

/**
  *  D 9 b for TriggerFanOut
  * \anchor testInitialValueThreaddedFanOut_D_9_b \ref initialValueThreaddedFanOut_D_9_b
  */
BOOST_AUTO_TEST_CASE(testTriggerFanOutInitValueAtDeviceD9) {
  std::cout << "===   testTriggerFanOutInitValueAtDeviceD9   === " << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  {
    TriggerFanOutInitialValueEceptionDummy dummyToStopTimeUntilOpen;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeUntilOpen.application.run();
    dummyToStopTimeUntilOpen.application.triggerModule.trigger.trigger.write();
    dummyToStopTimeUntilOpen.application.pushModuleD9_1.p.get_future().wait();
    dummyToStopTimeUntilOpen.application.pushModuleD9_2.p.get_future().wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    end = std::chrono::steady_clock::now();
  }
  {
    TriggerFanOutInitialValueEceptionDummy d;
    d.deviceBackend->throwExceptionOpen = true;
    BOOST_CHECK_THROW(d.deviceBackend->open(), std::exception);
    d.application.run();
    BOOST_CHECK(d.application.pushModuleD9_1.enteredTheMainLoop == false);
    std::this_thread::sleep_for(2 * (end - start));
    BOOST_CHECK(d.application.pushModuleD9_1.enteredTheMainLoop == false);
    BOOST_CHECK(d.pushVariable1.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.deviceBackend->throwExceptionOpen = false;
    d.application.triggerModule.trigger.trigger.write();
    d.application.pushModuleD9_1.p.get_future().wait();
    BOOST_CHECK(d.application.pushModuleD9_1.enteredTheMainLoop == true);
    BOOST_CHECK(d.pushVariable1.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
  }
}

BOOST_AUTO_TEST_SUITE_END()
