/**Example to simulate the working and usage of StatusMonitor*/

#include "ApplicationCore.h"
#include "StatusMonitor.h"
namespace ctk = ChimeraTK;
struct Simulation : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
/** All the variable names like "TEMPERATURE", MAX_STATUS,
 * MAX_MONITOR.WARNING.THRESHOLD and others should be exactly the same as
 * and their data type should match with those given in the the monitor.
 * The direction should be oppisite e.g., the ScalarPushInput<double_t> watch
 * in the StatusMonitor would become ctk::ScalarOutput<double_t> watch.
 * */

/**The value to be monitored.*/
   ctk::ScalarOutput<double_t> temperature{this, "TEMPERATURE", "deg", ""};
/**The status of the MaxMonitor*/
  ctk::ScalarPushInput<uint16_t> max_status{this, "MAX_STATUS", "", ""};
/**Thersholds for the MaxMonitor. The variable names are exactly as in MaxMonitor.*/
  ctk::ScalarOutput<double_t> max_warning{this, "MAX_MONITOR.WARNING.THRESHOLD", "",""};
  ctk::ScalarOutput<double_t> max_error{this, "MAX_MONITOR.ERROR.THRESHOLD", "",""};

/**The status of the MinMonitor.*/
  ctk::ScalarPushInput<uint16_t> min_status{this, "MIN_STATUS", "", ""};
/**Thersholds for the MinMonitor. The variable names are exactly as in MinMonitor.*/
  ctk::ScalarOutput<double_t> min_warning{this, "MIN_MONITOR.WARNING.THRESHOLD", "",""};
  ctk::ScalarOutput<double_t> min_error{this, "MIN_MONITOR.ERROR.THRESHOLD", "",""};

/**The status of the RangeMonitor.*/
  ctk::ScalarPushInput<uint16_t> range_status{this, "RANGE_STATUS", "", ""};
/**Thersholds for the MinMonitor. The variable names are exactly as in RangeMonitor.*/
  ctk::ScalarOutput<double_t> rangeWarningUpperLimit{this, "RANGE_MONITOR.WARNING.UPPER_LIMIT", "",""};
  ctk::ScalarOutput<double_t> rangeWarningLowerLimit{this, "RANGE_MONITOR.WARNING.LOWER_LIMIT", "",""};
  ctk::ScalarOutput<double_t> rangeErrorUpperLimit{this, "RANGE_MONITOR.ERROR.UPPER_LIMIT", "",""};
  ctk::ScalarOutput<double_t> rangeErrorLowerLimit{this, "RANGE_MONITOR.ERROR.LOWER_LIMIT", "",""};
  
  void mainLoop() {
/**Intialize temperature.*/
    temperature = 0;
    temperature.write(); 

/**Set MaxMonitor threshold.*/
    max_warning = 20;
    max_warning.write();
    max_error = 40;
    max_error.write();

/**Set MinMonitor threshold.*/
    min_warning = -20;
    min_warning.write();
    min_error = -40;
    min_error.write();
    
/**Set RangeMonitor threshold.*/
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
/**Create MaxMonitor with exact variable names as in simulation.*/
  ctk::MaxMonitor maxMonitor{this, "SIMULATION","",ChimeraTK::HierarchyModifier::none,"TEMPERATURE","MAX_STATUS",{"CS"}};
/**Create MinMonitor with exact variable names as in simulation.*/
  ctk::MinMonitor minMonitor{this, "SIMULATION","",ChimeraTK::HierarchyModifier::none,"TEMPERATURE","MIN_STATUS",{"CS"}};
/**Create RangeMonitor with exact variable names as in simulation.*/
  ctk::RangeMonitor rangeMonitor{this, "SIMULATION","",ChimeraTK::HierarchyModifier::none,"TEMPERATURE","RANGE_STATUS",{"CS"}};
  void defineConnections();
};
ExampleApp theExampleApp;
void ExampleApp::defineConnections() {
  ChimeraTK::setDMapFilePath("dummy.dmap");

/**Find and map all the variable with CS tag automatically.*/
  findTag("CS").connectTo(cs);
  
  dumpConnections();
  dumpConnectionGraph();
  dumpGraph();
  dumpModuleGraph("module-graph.dot");
  
}

