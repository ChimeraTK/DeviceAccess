#pragma once

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/DummyRegisterAccessor.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "DeviceModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "check_timeout.h"

#include <future>


struct PollModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPollInput<int> pollInput{this, "REG1", "", "", {"DEVICE"}};
  std::promise<void> p;
  void mainLoop() override {
    p.set_value();
  }
};

struct PushModule : ChimeraTK::ApplicationModule {
  using ChimeraTK::ApplicationModule::ApplicationModule;
  struct : ChimeraTK::VariableGroup {
    using ChimeraTK::VariableGroup::VariableGroup;
    ChimeraTK::ScalarPushInput<int> pushInput{this, "PUSH_READ", "", ""};
  } reg1{this, "REG1", ""};

  std::promise<void> p;
  void mainLoop() override {
    p.set_value();
  }
};

struct UpdateModule:ChimeraTK::ApplicationModule{
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarOutput<int> deviceRegister{this, "REG1", "", "", {"DEVICE"}};
  std::promise<void> p;
  void mainLoop() override {
    p.set_value();
  }
};

struct DummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  DummyApplication() : Application("DummyApplication") {}
  ~DummyApplication() { shutdown(); }

  PushModule pushModule{this, "", ""};
  PollModule pollModule{this, "", ""};
  UpdateModule updateModule{this, "", ""};

  ChimeraTK::ControlSystemModule cs;
  ChimeraTK::DeviceModule device{this, ExceptionDummyCDD1};

  void defineConnections() override {
    findTag("CS").connectTo(cs);
    findTag("DEVICE").connectTo(device);

    auto push_input = device("REG1/PUSH_READ", typeid(int), 1, ChimeraTK::UpdateMode::push);
    push_input >> pushModule.reg1.pushInput;
  }
};

template<bool enableTestFacility>
struct fixture_with_poll_and_push_input {
  fixture_with_poll_and_push_input()
  : deviceBackend(boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
        ChimeraTK::BackendFactory::getInstance().createBackend(DummyApplication::ExceptionDummyCDD1))),
    exceptionDummyRegister(deviceBackend->getRawAccessor("", "REG1")) {
    deviceBackend->open();
    testFacitiy.runApplication();


    status.replace(testFacitiy.getScalar<int>(
        ChimeraTK::RegisterPath("/Devices") / DummyApplication::ExceptionDummyCDD1 / "status"));
    message.replace(testFacitiy.getScalar<std::string>(
        ChimeraTK::RegisterPath("/Devices") / DummyApplication::ExceptionDummyCDD1 / "message"));

    //
    //  workaround to ensure we exit only after initial value propagation is complete. (meaning we
    //  are in the main loop of all modules)
    /************************************************************************************************/
    application.pollModule.p.get_future().wait();
    application.pushModule.p.get_future().wait();
    application.updateModule.p.get_future().wait();
    /************************************************************************************************/
  }

  ~fixture_with_poll_and_push_input() {
    deviceBackend->throwExceptionRead = false;
    deviceBackend->throwExceptionWrite = false;
  }

  template<typename T>
  auto read(ChimeraTK::DummyRegisterRawAccessor& accessor) {
    auto lock = accessor.getBufferLock();
    return static_cast<T>(accessor);
  }
  template<typename T>
  auto read(ChimeraTK::DummyRegisterRawAccessor&& accessor) {
    read<T>(accessor);
  }

  template<typename T>
  void write(ChimeraTK::DummyRegisterRawAccessor& accessor, T value) {
    auto lock = accessor.getBufferLock();
    accessor = static_cast<int32_t>(value);
  }
  template<typename T>
  void write(ChimeraTK::DummyRegisterRawAccessor&& accessor, T value) {
    write(accessor, value);
  }

  bool isDeviceInError() {
    // workaround: wait till device module recovey completes; assumption: status variable == 0 =>
    // device recovered.
    status.readLatest();
    return static_cast<int>(status);
  }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  DummyApplication application;
  ChimeraTK::TestFacility testFacitiy{enableTestFacility};

  ChimeraTK::ScalarRegisterAccessor<int> status;
  ChimeraTK::ScalarRegisterAccessor<std::string> message;
  ChimeraTK::DummyRegisterRawAccessor exceptionDummyRegister;

  ChimeraTK::ScalarPushInput<int>& pushVariable{application.pushModule.reg1.pushInput};
  ChimeraTK::ScalarPollInput<int>& pollVariable{application.pollModule.pollInput};
  ChimeraTK::ScalarOutput<int>& outputVariable{application.updateModule.deviceRegister};
};
