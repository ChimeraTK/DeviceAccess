#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
namespace ChimeraTK {
   
  enum States { OFF, OK, WARNING, ERROR };
  
  struct StatusMonitor : public ChimeraTK::ApplicationModule {
    
    /*StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, ChimeraTK::HierarchyModifier modifier,
        const std::string& input,
        const std::string& output,
        const std::unordered_set<std::string>& inputTags = {},
        const std::unordered_set<std::string>& parameterTags = {},
        const std::unordered_set<std::string>& outputTags = {});*/
        
    StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, ChimeraTK::HierarchyModifier modifier,
        const std::string& input,
        const std::string& output,
        const std::unordered_set<std::string>& _defaultTags);
        
    ~StatusMonitor() override {}
    void prepare() override {}
    std::string _input;
    std::string _output;
    std::unordered_set<std::string> _defaultTags;
    std::unordered_set<std::string> _inputTags;
    std::unordered_set<std::string> _parameterTags;
    std::unordered_set<std::string> _outputTags;
   ChimeraTK::ScalarPushInput<double_t> watch;
   ChimeraTK::ScalarOutput<uint16_t> status; 
    
  };
  
  
  struct MaxMonitor : public ChimeraTK::StatusMonitor {

    using StatusMonitor::StatusMonitor;

    ChimeraTK::ScalarPushInput<double_t> warning{this, "MAX_MONITOR.WARNING.THRESHOLD", "","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> error{this,"MAX_MONITOR.ERROR.THRESHOLD","","",_parameterTags};
    
    void mainLoop() {
    
      ReadAnyGroup group{watch, warning, error};
      while(true){
        group.readAny();
        if (watch > error){
          status = ERROR;
        }
        else if (watch > warning){
          status = WARNING;
        }
        else{ 
          status = OK;
        }
        status.write();
        usleep(500);
      }
    }
  };
  
  struct MinMonitor : public ChimeraTK::StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ChimeraTK::ScalarPushInput<double_t> warning{this, "MIN_MONITOR.WARNING.THRESHOLD", "","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> error{this,"MIN_MONITOR.ERROR.THRESHOLD","","",_parameterTags};
    void mainLoop() {
      ReadAnyGroup group{watch, warning, error};
      while(true){
        group.readAny();
        if (watch < error){
          status = ERROR;
        }
        else if (watch < warning){
          status = WARNING;
        }
        else{ 
          status = OK;
        }
        status.write();
        usleep(500);
      }
    }
  };
  
  struct RangeMonitor : public ChimeraTK::StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ChimeraTK::ScalarPushInput<double_t> warningUpperLimit{this, "RANGE_MONITOR.WARNING.UPPER_LIMIT", "","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> warningLowerLimit{this, "RANGE_MONITOR.WARNING.LOWER_LIMIT", "","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> errorUpperLimit{this,"RANGE_MONITOR.ERROR.UPPER_LIMIT","","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> errorLowerLimit{this,"RANGE_MONITOR.ERROR.LOWER_LIMIT","","",_parameterTags};
     
    void mainLoop() {
      
      ReadAnyGroup group{watch, warningUpperLimit, warningLowerLimit,
        errorUpperLimit, errorLowerLimit};
      while(true){
        group.readAny();
        double_t warningUpperLimitCorrected = warningUpperLimit;
        double_t errorUpperLimitCorrected = errorUpperLimit;
        
        
        if (warningUpperLimitCorrected < warningLowerLimit){
          warningUpperLimitCorrected = warningLowerLimit;
        }
        if (errorUpperLimitCorrected < errorLowerLimit){
          errorUpperLimitCorrected = errorLowerLimit;
        }
        if (watch <= errorUpperLimitCorrected && watch >= errorLowerLimit){
          status = ERROR;
        }
        else if (watch <= warningUpperLimitCorrected && watch >= warningLowerLimit){
          status = WARNING;
        }
        else{ 
          status = OK;
        }
        status.write();
        usleep(500);
      }
    }
  };
  
  struct ExactMonitor : public ChimeraTK::StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ChimeraTK::ScalarPushInput<double_t> requiredValue{this, "EXACT_MONITOR.REQUIRED_VALUE", "","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> watch{this, _input, "", "", _inputTags}; 
    ChimeraTK::ScalarOutput<uint16_t> status {this, _output,"","", _outputTags};
    void mainLoop() {
      ReadAnyGroup group{watch, requiredValue};
      while(true){
        group.readAny();
        if (watch != requiredValue){
          status = ERROR;
        }
        else{ 
          status = OK;
        }
        status.write();
        usleep(500);
      }
    }
  };
  
  struct StateMonitor : public ChimeraTK::StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ChimeraTK::ScalarPushInput<int> state{this, "STATE_MONITOR.ON", "","",_parameterTags};
    ChimeraTK::ScalarPushInput<double_t> watch{this, _input, "", "", _inputTags}; 
    ChimeraTK::ScalarOutput<uint16_t> status {this, _output,"","", _outputTags};
    void mainLoop() {
      ReadAnyGroup group{watch, state};
      while(true){
        group.readAny();
        if (watch != state){
          status = ERROR;
        }
        else if (state == OK || state == OFF) {
          status = state;
        }
        status.write();
        usleep(500);
      }
    }
  };
 
}
