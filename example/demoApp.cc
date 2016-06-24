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

    SCALAR_INPUT(double, operatorSetpoint, "MV/m", ctk::UpdateMode::poll);
    SCALAR_OUTPUT(double, loopSetpoint, "MV/m");

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

    SCALAR_INPUT(double, setpoint, "MV/m", ctk::UpdateMode::push);
    SCALAR_INPUT(double, readback, "MV/m", ctk::UpdateMode::push);
    SCALAR_OUTPUT(double, actuator, "MV/m");

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


class SimulatorModule : public ctk::ApplicationModule {
  public:

    SCALAR_INPUT(double, actuator, "MV/m", ctk::UpdateMode::push);
    SCALAR_OUTPUT(double, readback, "MV/m");

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
    using Application::Application;

    AutomationModule automation;
    ControlLoopModule controlLoop;
    SimulatorModule simulator;

    void initialise() {
      mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

      automation.operatorSetpoint.consumeFromControlSystem("MyLocation/setpoint");
      //connect(automation.operatorSetpoint, feedByControlSystem("MyLocation/setpoint"));
      automation.loopSetpoint.connectTo(controlLoop.setpoint);
      automation.loopSetpoint.feedToControlSystem("MyLocation/setpoint_automation");

      controlLoop.actuator.feedToDevice("Dummy0","/MyModule/Variable");
      controlLoop.actuator.feedToControlSystem("MyLocation/actuatorLoop");

      simulator.actuator.consumeFromDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
      simulator.actuator.addTrigger(controlLoop.actuator);
      simulator.actuator.feedToControlSystem("MyLocation/actuatorSimulator");

      // this will create an independent variable network, thus also another accessor to the device
      feedDeviceRegisterToControlSystem<double>("Dummy0","/MyModule/Variable", "MyLocation/actuatorSimulator_direct", controlLoop.actuator);

      // this is the same as above
      connect(feedByDevice<double>("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll),
              consumeByControlSystem("MyLocation/actuatorSimulator_direct2")
             ).addTrigger(findNetwork(&controlLoop.actuator));

      simulator.readback.connectTo(controlLoop.readback);
      simulator.readback.feedToControlSystem("MyLocation/readback");
      simulator.readback.feedToControlSystem("MyLocation/readback_another_time");

      dumpConnections();
    }

    virtual ~MyApp() {};

};

MyApp myApp("demoApp");

