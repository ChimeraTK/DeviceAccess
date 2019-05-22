/*!
 * \author Nadeem Shehzad (DESY)
 * \date 15.05.2019
 * \page statusmonitordoc Status Monitor
 *
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
 *  -  OFF, OK, WARNING, ERROR.
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
  template <typename T>
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
    ScalarPushInput<T> watch;
/**One of four possible states to be reported*/
    ScalarOutput<uint16_t> status; 
  };
  
/** Module for status monitoring depending on a maximum threshold value*/
  template <typename T>
  struct MaxMonitor : public StatusMonitor<T> {

    using StatusMonitor<T>::StatusMonitor;
/** WARNING state to be reported if threshold is crossed*/
    ScalarPushInput<T> warning{this, "MAX_MONITOR.WARNING.THRESHOLD", "","", StatusMonitor<T>::_parameterTags};
/** ERROR state to be reported if threshold is crossed*/
    ScalarPushInput<T> error{this,"MAX_MONITOR.ERROR.THRESHOLD","","",StatusMonitor<T>::_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::watch, warning, error};
      while(true){
        group.readAny();
        if (StatusMonitor<T>::watch > error){
          StatusMonitor<T>::status = ERROR;
        }
        else if (StatusMonitor<T>::watch > warning){
          StatusMonitor<T>::status = WARNING;
        }
        else{ 
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        usleep(500);
      }
    }
  };

/** Module for status monitoring depending on a minimum threshold value*/
  template <typename T>
  struct MinMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;

/** WARNING state to be reported if threshold is crossed*/
    ScalarPushInput<T> warning{this, "MIN_MONITOR.WARNING.THRESHOLD", "","",StatusMonitor<T>::_parameterTags};
/** ERROR state to be reported if threshold is crossed*/
    ScalarPushInput<T> error{this,"MIN_MONITOR.ERROR.THRESHOLD","","",StatusMonitor<T>::_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::watch, warning, error};
      while(true){
        group.readAny();
        if (StatusMonitor<T>::watch < error){
          StatusMonitor<T>::status = ERROR;
        }
        else if (StatusMonitor<T>::watch < warning){
          StatusMonitor<T>::status = WARNING;
        }
        else{ 
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
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
  template <typename T>
  struct RangeMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;

/** WARNING state to be reported if value is in between the upper and
 * lower limit including the start and end of thresholds.
 */
    ScalarPushInput<T> warningUpperLimit{this, "RANGE_MONITOR.WARNING.UPPER_LIMIT", "","",StatusMonitor<T>::_parameterTags};
    ScalarPushInput<T> warningLowerLimit{this, "RANGE_MONITOR.WARNING.LOWER_LIMIT", "","",StatusMonitor<T>::_parameterTags};
/** ERROR state to be reported if value is in between the upper and 
 * lower limit including the start and end of thresholds.
 */    
    ScalarPushInput<T> errorUpperLimit{this,"RANGE_MONITOR.ERROR.UPPER_LIMIT","","",StatusMonitor<T>::_parameterTags};
    ScalarPushInput<T> errorLowerLimit{this,"RANGE_MONITOR.ERROR.LOWER_LIMIT","","",StatusMonitor<T>::_parameterTags};
     
/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::watch, warningUpperLimit, warningLowerLimit,
        errorUpperLimit, errorLowerLimit};
      while(true){
        group.readAny();
        T warningUpperLimitCorrected = warningUpperLimit;
        T errorUpperLimitCorrected = errorUpperLimit;
/** The only sanity check done in this module is if the upper limit 
 * is less then lower limit*/                
        if (warningUpperLimitCorrected < warningLowerLimit){
          warningUpperLimitCorrected = warningLowerLimit;
        }
        if (errorUpperLimitCorrected < errorLowerLimit){
          errorUpperLimitCorrected = errorLowerLimit;
        }
        if (StatusMonitor<T>::watch <= errorUpperLimitCorrected && StatusMonitor<T>::watch >= errorLowerLimit){
          StatusMonitor<T>::status = ERROR;
        }
        else if (StatusMonitor<T>::watch <= warningUpperLimitCorrected && StatusMonitor<T>::watch >= warningLowerLimit){
          StatusMonitor<T>::status = WARNING;
        }
        else{ 
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        usleep(500);
      }
    }
  };
  
/** Module for status monitoring of an exact value.
 * If value monitored is not exactly the same. an error state will be
 * reported.*/  
  template <typename T>
  struct ExactMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;
/**ERROR state if value is not equal to requiredValue*/
    ScalarPushInput<T> requiredValue{this, "EXACT_MONITOR.REQUIRED_VALUE", "","",StatusMonitor<T>::_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in requiredValue, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::watch, requiredValue};
      while(true){
        group.readAny();
        if (StatusMonitor<T>::watch != requiredValue){
          StatusMonitor<T>::status = ERROR;
        }
        else{ 
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        usleep(500);
      }
    }
  };

/** Module for On/off status monitoring.
 * If value monitored is different then desired state (on/off) an error
 * will be reported, otherwise OFF(0) or OK(1) depending on state.
 */
  template <typename T>
  struct StateMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;
    
    ScalarPushInput<T> state{this, "STATE_MONITOR.STATE", "","",StatusMonitor<T>::_parameterTags};

/**This is where state evaluation is done*/
    void mainLoop() {
/** If there is a change either in value monitored or in state, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::watch, state};
      while(true){
        group.readAny();
        if (StatusMonitor<T>::watch != state){
          StatusMonitor<T>::status = ERROR;
        }
        else if (state == OK || state == OFF) {
          StatusMonitor<T>::status = state;
        }
        else {
          //no correct value
          StatusMonitor<T>::status = ERROR;
        }
        StatusMonitor<T>::status.write();
        usleep(500);
      }
    }
  };
}
