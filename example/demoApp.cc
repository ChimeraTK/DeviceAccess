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
    MyApp() : Application("demoApp") {}

    AutomationModule automation;
    ControlLoopModule controlLoop;
    SimulatorModule simulator;
    ctk::DeviceModule dev{"Dummy0", "MyModule"};

    void initialise() {
      mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

      ctrlVar("MyLocation/setpoint") >> automation.operatorSetpoint;
      automation.loopSetpoint >> controlLoop.setpoint
                              >> ctrlVar("MyLocation/setpoint_automation");

      controlLoop.actuator >> dev("Variable")
                           >> ctrlVar("MyLocation/actuatorLoop");

      dev("Variable") [ controlLoop.actuator ] >> simulator.actuator
                                               >> ctrlVar("MyLocation/actuatorSimulator");

      // this will create an independent variable network, thus also another accessor to the device
      dev("Variable", typeid(double)) [ controlLoop.actuator ] >> ctrlVar("MyLocation/actuatorSimulator_direct");

      simulator.readback >> controlLoop.readback
                         >> ctrlVar("MyLocation/readback")
                         >> ctrlVar("MyLocation/readback_another_time");

      dumpConnections();
    }
};

MyApp myApp;

