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

struct Automation : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;
    ctk::ScalarPollInput<double> opSP{this, "opSP", "MV", "..."};
    ctk::ScalarOutput<double> curSP{this, "curSP", "MV", "..."};
    ctk::ScalarPushInput<int> trigger{this, "trigger", "", "..."};
    
    void mainLoop() {
      while(true) {
        trigger.read();
        opSP.readLatest();     // opSP.read() would be equivalent
        if(std::abs(opSP - curSP) > 0.01) {
          curSP += std::max( std::min(opSP - curSP, 0.1), -0.1);
          curSP.write();
        }
      }
    }
};

constexpr size_t tableLength{16384};
constexpr double samplingFrequency = 9.0; // MHz
constexpr double bitScalingFactor = 2000.0; // bits/MV

struct TableGeneration : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;
    struct TableParameters : public ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<double> pulseLength{this, "pulseLength", "us", "..."};
      ctk::ScalarPushInput<double> setpoint{this, "setpoint", "MV", "..."};
    };
    TableParameters tableParameters{this, "tableParameters", "..."};
    
    ctk::ArrayOutput<int32_t> setpointTable{this, "setpointTable", "bits", tableLength, "..."};
    ctk::ArrayOutput<int32_t> feedforwardTable{this, "feedforwardTable", "bits", tableLength, "..."};
    
    void mainLoop() {
      while(true) {
        tableParameters.readAny();    // block until the any table parameter is changed
        
        for(size_t i = 0; i < tableLength; ++i) {
          if(i < tableParameters.pulseLength * samplingFrequency) {
            setpointTable[i] = tableParameters.setpoint * bitScalingFactor;
            feedforwardTable[i] = 0.5 * tableParameters.setpoint * bitScalingFactor;
          }
          else {
            setpointTable[i] = 0;
            feedforwardTable[i] = 0;
          }
        }
        
        setpointTable.write();
        feedforwardTable.write();
      }
    }
};

struct ExampleApp : public ctk::Application {
    ExampleApp() : Application("exampleApp") {}
    ~ExampleApp() { shutdown(); }

    Automation automation{this, "automation", "..."};
    TableGeneration tableGeneration{this, "tableGeneration", "..."};
    ctk::DeviceModule dev{"Device"};
    ctk::DeviceModule timer{"Timer"};
    ctk::ControlSystemModule cs{"MyLocation"};
    
    void defineConnections();
    
};
ExampleApp theExampleApp;


void ExampleApp::defineConnections() {
    mtca4u::setDMapFilePath("dummy.dmap");

    cs("setpoint") >> automation.opSP;
    automation.curSP >> tableGeneration.tableParameters.setpoint >> cs("currentSetpoint");
    
    auto macropulseNr = timer("macropulseNr", typeid(int), 1, ctk::UpdateMode::push);
    macropulseNr >> automation.trigger;
    
    cs("pulseLength") >> tableGeneration.tableParameters.pulseLength;
    
    tableGeneration.setpointTable >> dev("setpointTable");
    tableGeneration.feedforwardTable >> dev("feedforwardTable");
    
    dev("probeSignal", typeid(int), tableLength) [ macropulseNr ] >> cs("probeSignal");
    
    dumpConnections();
    dumpConnectionGraph();
    dumpGraph();
    dumpModuleGraph("module-graph.dot");
}

