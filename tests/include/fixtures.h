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

struct DummyApplication : ChimeraTK::Application {
  constexpr static const char* ExceptionDummyCDD1 = "(ExceptionDummy:1?map=test.map)";
  DummyApplication() : Application("DummyApplication") {}
  ~DummyApplication() { shutdown(); }

  PushModule pushModule{this, "", ""};
  PollModule pollModule{this, "", ""};

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
        ChimeraTK::BackendFactory::getInstance().createBackend(DummyApplication::ExceptionDummyCDD1))) {
    deviceBackend->open();
    testFacitiy.runApplication();

    exceptionDummyRegister.replace(application.device.device.getScalarRegisterAccessor<int>("/REG1"));
    status.replace(
        testFacitiy.getScalar<int>(ChimeraTK::RegisterPath("/Devices") / DummyApplication::ExceptionDummyCDD1 / "status"));
    message.replace(testFacitiy.getScalar<std::string>(
        ChimeraTK::RegisterPath("/Devices") / DummyApplication::ExceptionDummyCDD1 / "message"));

    //
    //  workaround to ensure we exit only after initial value propagation is complete. (meaning we
    //  are in the main loop of the modules)
    /************************************************************************************************/
    application.pollModule.p.get_future().wait();
    application.pushModule.p.get_future().wait();
    /************************************************************************************************/
  }

  ~fixture_with_poll_and_push_input() { deviceBackend->throwExceptionRead = false; }

  boost::shared_ptr<ChimeraTK::ExceptionDummy> deviceBackend;
  DummyApplication application;
  ChimeraTK::TestFacility testFacitiy{enableTestFacility};

  ChimeraTK::ScalarRegisterAccessor<int> status;
  ChimeraTK::ScalarRegisterAccessor<std::string> message;
  ChimeraTK::ScalarRegisterAccessor<int> exceptionDummyRegister;
};
