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

class AutomationModule : public ctk::ApplicationModule {
  public:

    SCALAR_ACCESSOR(double, operatorSetpoint, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(double, loopSetpoint, ctk::VariableDirection::feeding, "MV/m", ctk::UpdateMode::push);

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


class ControlLoopModule : public ctk::ApplicationModule {
  public:

    SCALAR_ACCESSOR(double, setpoint, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(double, readback, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(double, actuator, ctk::VariableDirection::feeding, "MV/m", ctk::UpdateMode::push);

    void mainLoop() {

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


class SimulatorModule : public ctk::ApplicationModule {
  public:

    SCALAR_ACCESSOR(double, actuator, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(double, readback, ctk::VariableDirection::feeding, "MV/m", ctk::UpdateMode::push);

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


class MyApp : public ctk::Application {

  public:

    AutomationModule automation;
    ControlLoopModule controlLoop;
    SimulatorModule simulator;

    void initialise() {
      mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

      automation.operatorSetpoint.consumeFromControlSystem("MyLocation/setpoint");
      automation.loopSetpoint.connectTo(controlLoop.setpoint);
      automation.loopSetpoint.feedToControlSystem("MyLocation/setpoint_automation");

      controlLoop.actuator.feedToDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
      controlLoop.actuator.feedToControlSystem("MyLocation/actuatorLoop");

      simulator.actuator.consumeFromDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
      //simulator.actuator.feedToControlSystem("MyLocation/actuatorSimulator"); // not allowed, since simulator.actuator consuming

      feedDeviceRegisterToControlSystem<double>("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll, "MyLocation/actuatorSimulator");

      simulator.readback.connectTo(controlLoop.readback);
      simulator.readback.feedToControlSystem("MyLocation/readback");
      simulator.readback.feedToControlSystem("MyLocation/readback_another_time");
    }

    virtual ~MyApp() {};

};

MyApp myApp;

