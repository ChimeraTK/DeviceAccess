#include "ApplicationCore.h"
#include "StatusMonitor.h"
namespace ctk = ChimeraTK;
struct Simulation : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarOutput<double_t> temperature{this, "TEMPERATURE", "deg", ""};
  ctk::ScalarPushInput<uint16_t> max_status{this, "MAX_STATUS", "", ""};
  ctk::ScalarOutput<double_t> max_warning{this, "MAX_MONITOR.WARNING.THRESHOLD", "",""};
  ctk::ScalarOutput<double_t> max_error{this, "MAX_MONITOR.ERROR.THRESHOLD", "",""};
  
  ctk::ScalarPushInput<uint16_t> min_status{this, "MIN_STATUS", "", ""};
  ctk::ScalarOutput<double_t> min_warning{this, "MIN_MONITOR.WARNING.THRESHOLD", "",""};
  ctk::ScalarOutput<double_t> min_error{this, "MIN_MONITOR.ERROR.THRESHOLD", "",""};
  
  ctk::ScalarPushInput<uint16_t> range_status{this, "RANGE_STATUS", "", ""};
  ctk::ScalarOutput<double_t> rangeWarningUpperLimit{this, "RANGE_MONITOR.WARNING.UPPER_LIMIT", "",""};
  ctk::ScalarOutput<double_t> rangeWarningLowerLimit{this, "RANGE_MONITOR.WARNING.LOWER_LIMIT", "",""};
  ctk::ScalarOutput<double_t> rangeErrorUpperLimit{this, "RANGE_MONITOR.ERROR.UPPER_LIMIT", "",""};
  ctk::ScalarOutput<double_t> rangeErrorLowerLimit{this, "RANGE_MONITOR.ERROR.LOWER_LIMIT", "",""};
  
  
  

  void mainLoop() {
    temperature = 0;
    temperature.write(); 
    
    max_warning = 20;
    max_warning.write();
    max_error = 40;
    max_error.write();
    
    min_warning = -20;
    min_warning.write();
    min_error = -40;
    min_error.write();
    
    rangeWarningLowerLimit = 21;
    rangeWarningLowerLimit.write();
    rangeWarningUpperLimit = 35;
    rangeWarningUpperLimit.write();
    rangeErrorLowerLimit = 36;
    rangeErrorLowerLimit.write();
    rangeErrorUpperLimit = 70;
    rangeErrorUpperLimit.write();
    
    while(true) {
      //this is simulating temperature going high and low.
      if (temperature > 50)
      {
        while(temperature > -50)
        {
          temperature -=1;
          temperature.write();
          usleep(100000);
          min_status.read();
          max_status.read();
          range_status.read();
          std::cout<<"temperature:"<<temperature<<" min_status:"<<min_status 
          <<" max_status:"<<max_status<<" range_status:"<<range_status<<std::endl;
        }
      }
      temperature += 1;
      temperature.write(); 
      usleep(100000);
      min_status.read();
      max_status.read();
      range_status.read();
      std::cout<<"temperature:"<<temperature<<" min_status:"<<min_status
      <<" max_status:"<<max_status<<" range_status:"<<range_status<<std::endl;
      
    }
  }
};


struct ExampleApp : public ctk::Application {
  ExampleApp() : Application("exampleApp") {}
  ~ExampleApp() { shutdown(); }

  Simulation simulation{this, "SIMULATION", "",ChimeraTK::HierarchyModifier::none,{"CS"}};
  ctk::ControlSystemModule cs;
  ctk::MaxMonitor maxMonitor{this, "SIMULATION","",ChimeraTK::HierarchyModifier::none,"TEMPERATURE","MAX_STATUS",{"CS"}};
  ctk::MinMonitor minMonitor{this, "SIMULATION","",ChimeraTK::HierarchyModifier::none,"TEMPERATURE","MIN_STATUS",{"CS"}};
  ctk::RangeMonitor rangeMonitor{this, "SIMULATION","",ChimeraTK::HierarchyModifier::none,"TEMPERATURE","RANGE_STATUS",{"CS"}};
  void defineConnections();
};
ExampleApp theExampleApp;
void ExampleApp::defineConnections() {
  ChimeraTK::setDMapFilePath("dummy.dmap");
  
  findTag("CS").connectTo(cs);
  
  dumpConnections();
  dumpConnectionGraph();
  dumpGraph();
  dumpModuleGraph("module-graph.dot");
  
}

