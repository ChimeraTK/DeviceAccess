#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
namespace ChimeraTK {
   
  enum States { OFF, OK, WARNING, ERROR };
  
  struct StatusMonitor : public ApplicationModule {
    
     StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, HierarchyModifier modifier,
        const std::string& input,
        const std::string& output,
        const std::unordered_set<std::string>& tags)
        : ApplicationModule(owner, name, description, modifier),
          _parameterTags(tags), watch(this, input, "", "", tags),
          status(this, output,"","",tags){}
    
    
    StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, HierarchyModifier modifier,
        const std::string& input,
        const std::unordered_set<std::string>& inputTags,
        const std::string& output,
        const std::unordered_set<std::string>& outputTags,
        const std::unordered_set<std::string>& parameterTags)
         : ApplicationModule(owner, name, description, modifier),
          _parameterTags(parameterTags), watch(this, input, "", "", inputTags),
          status(this, output,"","",outputTags){}
        
    StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, HierarchyModifier modifier,
        const std::string& input,
        const std::unordered_set<std::string>& inputTags,
        const std::string& output,
        const std::unordered_set<std::string>& parameterTags)
         : ApplicationModule(owner, name, description, modifier),
          _parameterTags(parameterTags), watch(this, input, "", "", inputTags),
          status(this, output,"","",{}){}
   
   StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, HierarchyModifier modifier,
        const std::string& input,
        const std::string& output,
        const std::unordered_set<std::string>& inputTags,
        const std::unordered_set<std::string>& outputTags)
         : ApplicationModule(owner, name, description, modifier),
          _parameterTags({}), watch(this, input, "", "", inputTags),
          status(this, output,"","",outputTags){}             
        
    ~StatusMonitor() override {}
    void prepare() override {}
    std::unordered_set<std::string> _parameterTags;
    ScalarPushInput<double_t> watch;
    ScalarOutput<uint16_t> status; 
    
  };
  
  
  struct MaxMonitor : public StatusMonitor {

    using StatusMonitor::StatusMonitor;
          
    ScalarPushInput<double_t> warning{this, "MAX_MONITOR.WARNING.THRESHOLD", "","",_parameterTags};
    ScalarPushInput<double_t> error{this,"MAX_MONITOR.ERROR.THRESHOLD","","",_parameterTags};
    
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
  
  struct MinMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ScalarPushInput<double_t> warning{this, "MIN_MONITOR.WARNING.THRESHOLD", "","",_parameterTags};
    ScalarPushInput<double_t> error{this,"MIN_MONITOR.ERROR.THRESHOLD","","",_parameterTags};
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
  
  struct RangeMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ScalarPushInput<double_t> warningUpperLimit{this, "RANGE_MONITOR.WARNING.UPPER_LIMIT", "","",_parameterTags};
    ScalarPushInput<double_t> warningLowerLimit{this, "RANGE_MONITOR.WARNING.LOWER_LIMIT", "","",_parameterTags};
    ScalarPushInput<double_t> errorUpperLimit{this,"RANGE_MONITOR.ERROR.UPPER_LIMIT","","",_parameterTags};
    ScalarPushInput<double_t> errorLowerLimit{this,"RANGE_MONITOR.ERROR.LOWER_LIMIT","","",_parameterTags};
     
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
  
  struct ExactMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ScalarPushInput<double_t> requiredValue{this, "EXACT_MONITOR.REQUIRED_VALUE", "","",_parameterTags};
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
  
  struct StateMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ScalarPushInput<int> state{this, "STATE_MONITOR.ON", "","",_parameterTags};
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
