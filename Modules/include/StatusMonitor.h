/*!
 * \author Nadeem Shehzad (DESY)
 * \date 15.05.2019
 * \page statusmonitordoc Status Monitor
 * \section Introduction
 *
 * To monitor a status of a varaible in an appplicaiton this group of
 * modules provides different possiblites.
 * It includes
 *  - MaxMonitor to monitor a value depending upon two MAX thresholds for warning and error.
 *  - MinMonitor to monitor a value depending upon two MIN thresholds for warning and error.
 *  - RangeMonitor to monitor a value depending upon two ranges of thresholds for warning and error.
 *  - ExactMonitor to monitor a value which should be exactly same as required value.
 *  - StateMonitor to monitor On/Off state.
 * Depending upon the value and condition on of the four states are reported.
 * -  OFF, OK, WARNING, ERROR.
 *
 * Checkout demoStatusMonitor.cc under status_monitor_example to see in
 * detail how it works.
 */

/** Generic modules for status monitoring.
 * Each module monitors an input variable and depending upon the
 * conditions reports four different states.
*/
#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
namespace ChimeraTK {

/** There are four states that can be reported*/
  enum States { OFF, OK, WARNING, ERROR };
  
  struct StatusMonitor : public ApplicationModule {

/** Number of convience constructors for ease of use.
 * The input and output variable names can be given by user which
 * should be mapped with the variables of module to be watched.
*/

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

/**Tags for parameters. This makes it easier to connect them to e.g, to control system*/
    std::unordered_set<std::string> _parameterTags;
/**Input value that should be monitored*/
//TODO we might need to template this?
    ScalarPushInput<double_t> watch;
/**One of four possible states to be reported*/
    ScalarOutput<uint16_t> status; 
  };
  

/** Module for status monitoring depending on a maximum threshold value*/
  struct MaxMonitor : public StatusMonitor {

    using StatusMonitor::StatusMonitor;
/** WARNING state to be reported if threshold is crossed*/
    ScalarPushInput<double_t> warning{this, "MAX_MONITOR.WARNING.THRESHOLD", "","",_parameterTags};
/** ERROR state to be reported if threshold is crossed*/
    ScalarPushInput<double_t> error{this,"MAX_MONITOR.ERROR.THRESHOLD","","",_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
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

/** Module for status monitoring depending on a minimum threshold value*/
  struct MinMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;

/** WARNING state to be reported if threshold is crossed*/
    ScalarPushInput<double_t> warning{this, "MIN_MONITOR.WARNING.THRESHOLD", "","",_parameterTags};
/** ERROR state to be reported if threshold is crossed*/
    ScalarPushInput<double_t> error{this,"MIN_MONITOR.ERROR.THRESHOLD","","",_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
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

/** Module for status monitoring depending on range of threshold values.
 * As long as a monitored value is in the range defined by user it goes
 * to error or warning state. If the monitored value exceeds the upper limmit
 * or goes under the lowerlimit the state reported will be always OK.
 * IMPORTANT: This module does not check for ill logic, so make sure to
 * set the ranges correctly to issue warning or error.
 */
  struct RangeMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;

/** WARNING state to be reported if value is in between the upper and
 * lower limit including the start and end of thresholds.
 */
    ScalarPushInput<double_t> warningUpperLimit{this, "RANGE_MONITOR.WARNING.UPPER_LIMIT", "","",_parameterTags};
    ScalarPushInput<double_t> warningLowerLimit{this, "RANGE_MONITOR.WARNING.LOWER_LIMIT", "","",_parameterTags};
/** ERROR state to be reported if value is in between the upper and 
 * lower limit including the start and end of thresholds.
 */    
    ScalarPushInput<double_t> errorUpperLimit{this,"RANGE_MONITOR.ERROR.UPPER_LIMIT","","",_parameterTags};
    ScalarPushInput<double_t> errorLowerLimit{this,"RANGE_MONITOR.ERROR.LOWER_LIMIT","","",_parameterTags};
     
/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{watch, warningUpperLimit, warningLowerLimit,
        errorUpperLimit, errorLowerLimit};
      while(true){
        group.readAny();
        double_t warningUpperLimitCorrected = warningUpperLimit;
        double_t errorUpperLimitCorrected = errorUpperLimit;
/** The only sanity check done in this module is if the upper limit 
 * is less then lower limit*/                
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
  
/** Module for status monitoring of an exact value.
 * If value monitored is not exactly the same. an error state will be
 * reported.*/  
  struct ExactMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;
/**ERROR state if value is not equal to requiredValue*/
    ScalarPushInput<double_t> requiredValue{this, "EXACT_MONITOR.REQUIRED_VALUE", "","",_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in requiredValue, the status is re-evaluated*/
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

/** Module for On/off status monitoring.
 * If value monitored is different then desired state (on/off) an error
 * will be reported, otherwise OFF(0) or OK(1) depending on state.
 */
  struct StateMonitor : public StatusMonitor {
    using StatusMonitor::StatusMonitor;
    
    ScalarPushInput<int> state{this, "STATE_MONITOR.STATE", "","",_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in state, the status is re-evaluated*/
      ReadAnyGroup group{watch, state};
      while(true){
        group.readAny();
        if (watch != state){
          status = ERROR;
        }
        else if (state == OK || state == OFF) {
          status = state;
        }
        else {
          //no correct value
          status = ERROR;
        }
        status.write();
        usleep(500);
      }
    }
  };
 
}
