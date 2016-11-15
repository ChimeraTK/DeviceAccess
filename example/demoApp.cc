/*
 * demoApp.cc
 *
 *  Created on: Jun 9, 2016
 *      Author: Martin Hierholzer
 */

#include <iostream>

#include <mtca4u/BackendFactory.h>

#include "ApplicationCore.h"

namespace ctk = ChimeraTK;

struct AutomationModule : public ctk::ApplicationModule {
    AutomationModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    CTK_SCALAR_INPUT(double, operatorSetpoint, "Celsius", ctk::UpdateMode::poll, "Setpoint given by the operator");
    CTK_SCALAR_OUTPUT(double, loopSetpoint, "Celsius", "Setpoint computed by the automation module");

    void mainLoop() {
      loopSetpoint = 0;
      loopSetpoint.write();

      while(true) {
        operatorSetpoint.read();
        if(operatorSetpoint > loopSetpoint) loopSetpoint++;
        if(operatorSetpoint < loopSetpoint) loopSetpoint--;
        std::cout << "AutomationModule: operatorSetpoint = " << operatorSetpoint << std::endl;
        std::cout << "AutomationModule: loopSetpoint = " << loopSetpoint << std::endl;
        loopSetpoint.write();
        usleep(200000);
      }

    }
};


struct ControlLoopModule : public ctk::ApplicationModule {
    ControlLoopModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    CTK_SCALAR_INPUT(double, setpoint, "Celsius", ctk::UpdateMode::push, "Setpoint for my control loop");
    CTK_SCALAR_INPUT(double, readback, "Celsius", ctk::UpdateMode::push, "Control loop input value");
    CTK_SCALAR_OUTPUT(double, actuator, "A", "Actuator output of the control loop");

    void mainLoop() {

      actuator = 0;
      actuator.write();

      while(true) {
        readback.read();
        setpoint.read();
        actuator = 10.*(setpoint-readback);
        std::cout << "ControlLoopModule: setpoint = " << setpoint << std::endl;
        std::cout << "ControlLoopModule: readback = " << readback << std::endl;
        std::cout << "ControlLoopModule: actuator = " << actuator << std::endl;
        actuator.write();
        usleep(200000);
      }

    }
};


struct SimulatorModule : public ctk::ApplicationModule {
    SimulatorModule(ctk::EntityOwner *owner, const std::string &name) : ctk::ApplicationModule(owner,name) {}

    CTK_SCALAR_INPUT(double, actuator, "A", ctk::UpdateMode::push, "Actuator input for the simulation");
    CTK_SCALAR_OUTPUT(double, readback, "Celsius", "Readback output value of the simulation");

    double lastValue{0};

    void mainLoop() {

      while(true) {
        actuator.read();
        readback = (100*lastValue + actuator)/100.;
        lastValue = readback;
        std::cout << "SimulatorModule: actuator = " << actuator << std::endl;
        std::cout << "SimulatorModule: readback = " << readback << std::endl;
        readback.write();
        usleep(200000);
      }

    }
};


struct MyApp : public ctk::Application {
    MyApp() : Application("demoApp") {}
    ~MyApp() { shutdown(); }

    AutomationModule automation{this, "automation"};
    ControlLoopModule controlLoop{this, "controlLoop"};
    SimulatorModule simulator{this, "simulator"};
    ctk::DeviceModule dev{"Dummy0", "MyModule"};
    ctk::ControlSystemModule cs{"MyLocation"};

    void defineConnections() {
      mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

      cs("setpoint") >> automation.operatorSetpoint;
      automation.loopSetpoint >> controlLoop.setpoint >> cs("setpoint_automation");

      controlLoop.actuator >> dev("Variable") >> cs("actuatorLoop");

      // TODO Reading back from the device register is not working. Should it work, or should it throw an error? FIXME
      //dev("Variable") [ controlLoop.actuator ] >> simulator.actuator >> cs("actuatorSimulator");
      //dev("Variable", typeid(double)) [ controlLoop.actuator ] >> cs("actuatorSimulator_direct");
      controlLoop.actuator >> simulator.actuator >> cs("actuatorSimulator");

      simulator.readback >> controlLoop.readback >> cs("readback") >> cs("readback_another_time");

      // Proposed notations and special cases (not yet implemented):
      // dev("Some2DRegister") [ triggerVariable ] >> ( Channel(0) >> cavity0.probe, Channel(1) >> cavity1.probe );
      // dev("SomeBitField") [ triggerVariable ] >> ( Bit(0) >> myModule.someBoolean, Bit(31) >> anotherModule.yesOrNo );
      // cs("SomeVector") >> ( Element(3) >> myModule.someScalar, Elements(4,2) >> myModule.smallVector );
      // dev("Variable") [ cs("myFirstTrigger") ] >> myModuleA.Variable;
      // dev("Variable") [ cs("mySecondTrigger") ] >> myModuleB.Variable;

      dumpConnections();
    }

};

MyApp myApp;

