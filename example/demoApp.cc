/*
 * demoApp.cc
 *
 *  Created on: Jun 9, 2016
 *      Author: Martin Hierholzer
 */

#include <iostream>

#include "ApplicationCore.h"
#include "EnableXMLGenerator.h"

namespace ctk = ChimeraTK;

struct AutomationModule : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPollInput<double> operatorSetpoint{this, "operatorSetpoint", "Celsius", "Setpoint given by the operator"};
    ctk::ScalarOutput<double> loopSetpoint{this, "loopSetpoint", "Celsius", "Setpoint computed by the automation module"};

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
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInput<double> setpoint{this, "setpoint", "Celsius", "Setpoint for my control loop"};
    ctk::ScalarPushInput<double> readback{this, "readback", "Celsius", "Control loop input value"};
    ctk::ScalarOutput<double> actuator{this, "actuator", "A", "Actuator output of the control loop"};

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
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInput<double> actuator{this, "actuator", "A", "Actuator input for the simulation"};
    ctk::ScalarOutput<double> readback{this, "readback", "Celsius", "Readback output value of the simulation"};

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

    AutomationModule automation{this, "automation", "Module for automated ramping of setpoint"};
    ControlLoopModule controlLoop{this, "controlLoop", "Implements a control loop"};
    SimulatorModule simulator{this, "simulator", "Simulates a device"};
    ctk::DeviceModule dev{"Dummy0", "MyModule"};
    ctk::ControlSystemModule cs{"MyLocation"};

    void defineConnections() {
      mtca4u::setDMapFilePath("dummy.dmap");

      cs("setpoint") >> automation.operatorSetpoint;
      automation.loopSetpoint >> controlLoop.setpoint >> cs("setpoint_automation");

      controlLoop.actuator >> dev("actuator") >> cs("actuatorLoop");

      dev("readBack") [ controlLoop.actuator ] >> simulator.actuator >> cs("actuatorSimulator");
      dev("anotherReadBack", typeid(double), 1) [ controlLoop.actuator ] >> cs("actuatorSimulator_direct");

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

