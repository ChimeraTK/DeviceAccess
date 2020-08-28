#define BOOST_TEST_MODULE testInitialValues

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <future>
#include <functional>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/ExceptionDummyBackend.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
//#include "check_timeout.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

// A generic module with just one input. It is connected manually, so we just call the register "REG1" so we easily connect to that register in the device
// It has a flag and a promise to check whether the module has entered the main loop, and to wait for it.
template<class INPUT_TYPE>
struct InputModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  INPUT_TYPE input{this, "REG1", "", ""};
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

  InputModule<ctk::ScalarPollInput<int>> inputModule{this, "PollModule", ""};
  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override { inputModule.connectTo(device); }
};

// for the push type we need different connection code
struct PushDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  PushDummyApplication() : Application("DummyApplication") {}
  ~PushDummyApplication() override { shutdown(); }

  InputModule<ctk::ScalarPushInput<int>> inputModule{this, "PushModule", ""};
  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    auto push_register = device("REG1/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::push);
    push_register >> inputModule.input;
  }
};

template<class APPLICATION_TYPE>
struct TestFixtureWithEceptionDummy {
  TestFixtureWithEceptionDummy()
  : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(PollDummyApplication::ExceptionDummyCDD1))) {}
  ~TestFixtureWithEceptionDummy() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  APPLICATION_TYPE application;
  ChimeraTK::TestFacility testFacitiy{false};
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
};

/**
  *  Test Initial Values - Inputs of `ApplicationModule`s
  *  \anchor testInitialValuesInputsOfApplicationCore \ref InitialValuesInputsOfApplicationCore_D_8 "D.8"
  */
BOOST_AUTO_TEST_SUITE(testInitialValuesInputsOfApplicationCore_D_8)

typedef boost::mpl::list<PollDummyApplication, PushDummyApplication> DeviceTestApplicationTypes;

/**
  *  For device variables the ExeptionHandlingDecorator freezes the variable until the device is available
  * \anchor testInitialValue_D_8_b_i \ref initialValue_D_8_b_i
  */
BOOST_AUTO_TEST_CASE_TEMPLATE(testInitValueAtDevice8bi, APPLICATION_TYPE, DeviceTestApplicationTypes) {
  std::cout << "===   testInitValueAtDevice8bi " << typeid(APPLICATION_TYPE).name() << "  ===" << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  { // Here the time is stopped until you reach the mainloop.
    TestFixtureWithEceptionDummy<PollDummyApplication> dummyToStopTimeUntilOpen;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeUntilOpen.application.run();
    dummyToStopTimeUntilOpen.application.inputModule.p.get_future().wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    end = std::chrono::steady_clock::now();
  }
  { // waiting 2 x the time stoped above, in the assumption that it is then freezed,
    // as it is described in the spec.
    TestFixtureWithEceptionDummy<PollDummyApplication> d;
    d.deviceBackend->throwExceptionOpen = true;
    BOOST_CHECK_THROW(d.deviceBackend->open(), std::exception);
    d.application.run();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    BOOST_CHECK(d.application.inputModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * elapsed_milliseconds));
    BOOST_CHECK(d.application.inputModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.application.inputModule.input.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.deviceBackend->throwExceptionOpen = false;
    d.application.inputModule.p.get_future().wait();
    BOOST_CHECK(d.application.inputModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.application.inputModule.input.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
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

template<class INPUT_TYPE>
struct ProcessArryDummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  ProcessArryDummyApplication() : Application("DummyApplication") {}
  ~ProcessArryDummyApplication() override { shutdown(); }

  InputModule<INPUT_TYPE> inputModule{this, "PollModule", ""};
  ScalarOutputModule scalarOutputModule{this, "ScalarOutputModule", ""};
  void defineConnections() override { scalarOutputModule.connectTo(inputModule); }
};

typedef boost::mpl::list<ctk::ScalarPollInput<int>, ctk::ScalarPushInput<int>> TestInputTypes;

/**
  *  ProcessArray freeze in their implementation until the initial value is received
  * \anchor testInitialValue_D_8_b_ii \ref initialValue_D_8_b_ii
  */
BOOST_AUTO_TEST_CASE_TEMPLATE(testProcessArrayInitValueAtDevice8bii, INPUT_TYPE, TestInputTypes) {
  std::cout << "===   testPollProcessArrayInitValueAtDevice8bii " << typeid(INPUT_TYPE).name() << "  === " << std::endl;
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  {
    // we don't need the exception dummy in this test. But no need to write a new fixture for it.
    TestFixtureWithEceptionDummy<ProcessArryDummyApplication<INPUT_TYPE>> dummyToStopTimeForApplicationStart;
    start = std::chrono::steady_clock::now();
    dummyToStopTimeForApplicationStart.application.run();
    dummyToStopTimeForApplicationStart.application.scalarOutputModule.output.write();
    dummyToStopTimeForApplicationStart.application.inputModule.p.get_future().wait();
    end = std::chrono::steady_clock::now();
  }
  {
    TestFixtureWithEceptionDummy<ProcessArryDummyApplication<INPUT_TYPE>> d;
    d.application.run();
    BOOST_CHECK(d.application.inputModule.enteredTheMainLoop == false);
    std::this_thread::sleep_for(end - start);
    BOOST_CHECK(d.application.inputModule.enteredTheMainLoop == false);
    BOOST_CHECK(d.application.inputModule.input.getVersionNumber() == ctk::VersionNumber(std::nullptr_t()));
    d.application.scalarOutputModule.output.write();
    d.application.inputModule.p.get_future().wait();
    BOOST_CHECK(d.application.inputModule.enteredTheMainLoop == true);
    BOOST_CHECK(d.application.inputModule.input.getVersionNumber() != ctk::VersionNumber(std::nullptr_t()));
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
