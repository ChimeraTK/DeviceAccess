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
 * Checkout the status monitor example to see in detail how it works.
 * \include demoStatusMonitor.cc
 */

/**
For more info see \ref statusmonitordoc
\example demoStatusMonitor.cc
*/

/** Generic modules for status monitoring.
 * Each module monitors an input variable and depending upon the
 * conditions reports four different states.
*/
#include "ApplicationCore.h"
namespace ChimeraTK {

  /** There are four states that can be reported*/
  enum States { OFF, OK, WARNING, ERROR };
  template<typename T>
  struct StatusMonitor : public ApplicationModule {
    /** Number of convience constructors for ease of use.
 * The input and output variable names can be given by user which
 * should be mapped with the variables of module to be watched.
*/
    StatusMonitor(EntityOwner* owner, const std::string& name, const std::string& description, const std::string& input,
        const std::string& output, HierarchyModifier modifier, const std::unordered_set<std::string>& outputTags = {},
        const std::unordered_set<std::string>& parameterTags = {}, const std::unordered_set<std::string>& tags = {})
    : ApplicationModule(owner, name, description, modifier, tags), _parameterTags(parameterTags), oneUp(this, input),
      status(this, output, "", "", outputTags) {}

    StatusMonitor() { throw logic_error("Default constructor unusable. Just exists to work around gcc bug."); }

    ~StatusMonitor() override {}
    void prepare() override {}

    /**Tags for parameters. This makes it easier to connect them to e.g, to control system*/
    std::unordered_set<std::string> _parameterTags;
    /**Input value that should be monitored. It is moved one level up, so it's parallel to this monitor object.*/
    struct OneUp : public VariableGroup {
      OneUp(EntityOwner* owner, const std::string& watchName)
      : VariableGroup(owner, "hidden", "", HierarchyModifier::oneUpAndHide), watch(this, watchName, "", "") {}
      ScalarPushInput<T> watch;
    } oneUp;

    /**One of four possible states to be reported*/
    ScalarOutput<uint16_t> status;
  };

  /** Module for status monitoring depending on a maximum threshold value*/
  template<typename T>
  struct MaxMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;
    /** WARNING state to be reported if threshold is reached or exceeded*/
    ScalarPushInput<T> warning{this, "upperWarningThreshold", "", "", StatusMonitor<T>::_parameterTags};
    /** ERROR state to be reported if threshold is reached or exceeded*/
    ScalarPushInput<T> error{this, "upperErrorThreshold", "", "", StatusMonitor<T>::_parameterTags};

    /**This is where state evaluation is done*/
    void mainLoop() {
      /** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::oneUp.watch, warning, error};
      while(true) {
        // evaluate and publish first, then read and wait. This takes care of the publishing the initial variables
        if(StatusMonitor<T>::oneUp.watch >= error) {
          StatusMonitor<T>::status = ERROR;
        }
        else if(StatusMonitor<T>::oneUp.watch >= warning) {
          StatusMonitor<T>::status = WARNING;
        }
        else {
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        group.readAny();
      }
    }
  };

  /** Module for status monitoring depending on a minimum threshold value*/
  template<typename T>
  struct MinMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;

    /** WARNING state to be reported if threshold is crossed*/
    ScalarPushInput<T> warning{this, "lowerWarningThreshold", "", "", StatusMonitor<T>::_parameterTags};
    /** ERROR state to be reported if threshold is crossed*/
    ScalarPushInput<T> error{this, "lowerErrorThreshold", "", "", StatusMonitor<T>::_parameterTags};

    /**This is where state evaluation is done*/
    void mainLoop() {
      /** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::oneUp.watch, warning, error};
      while(true) {
        if(StatusMonitor<T>::oneUp.watch <= error) {
          StatusMonitor<T>::status = ERROR;
        }
        else if(StatusMonitor<T>::oneUp.watch <= warning) {
          StatusMonitor<T>::status = WARNING;
        }
        else {
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        group.readAny();
      }
    }
  };

  /** Module for status monitoring depending on range of threshold values.
 * As long as a monitored value is in the range defined by user it goes
 * to error or warning state. If the monitored value exceeds the upper limmit
 * or goes under the lowerthreshold the state reported will be always OK.
 * IMPORTANT: This module does not check for ill logic, so make sure to
 * set the ranges correctly to issue warning or error.
 */
  template<typename T>
  struct RangeMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;

    /** WARNING state to be reported if value is in between the upper and
 * lower threshold including the start and end of thresholds.
 */
    ScalarPushInput<T> warningUpperThreshold{this, "upperWarningThreshold", "", "", StatusMonitor<T>::_parameterTags};
    ScalarPushInput<T> warningLowerThreshold{this, "lowerWarningThreshold", "", "", StatusMonitor<T>::_parameterTags};
    /** ERROR state to be reported if value is in between the upper and
 * lower threshold including the start and end of thresholds.
 */
    ScalarPushInput<T> errorUpperThreshold{this, "upperErrorThreshold", "", "", StatusMonitor<T>::_parameterTags};
    ScalarPushInput<T> errorLowerThreshold{this, "lowerErrorThreshold", "", "", StatusMonitor<T>::_parameterTags};

    /**This is where state evaluation is done*/
    void mainLoop() {
      /** If there is a change either in value monitored or in thershold values, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::oneUp.watch, warningUpperThreshold, warningLowerThreshold,
          errorUpperThreshold, errorLowerThreshold};
      while(true) {
        // Check for error limits first. Like this they supersede the warning,
        // even if they are stricter then the warning limits (mis-configuration)
        if(StatusMonitor<T>::oneUp.watch <= errorLowerThreshold ||
            StatusMonitor<T>::oneUp.watch >= errorUpperThreshold) {
          StatusMonitor<T>::status = ERROR;
        }
        else if(StatusMonitor<T>::oneUp.watch <= warningLowerThreshold ||
            StatusMonitor<T>::oneUp.watch >= warningUpperThreshold) {
          StatusMonitor<T>::status = WARNING;
        }
        else {
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        group.readAny();
      }
    }
  };

  /** Module for status monitoring of an exact value.
 * If value monitored is not exactly the same. an error state will be
 * reported.*/
  template<typename T>
  struct ExactMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;
    /**ERROR state if value is not equal to requiredValue*/
    ScalarPushInput<T> requiredValue{this, "requiredValue", "", "", StatusMonitor<T>::_parameterTags};

    /**This is where state evaluation is done*/
    void mainLoop() {
      /** If there is a change either in value monitored or in requiredValue, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::oneUp.watch, requiredValue};
      while(true) {
        if(StatusMonitor<T>::oneUp.watch != requiredValue) {
          StatusMonitor<T>::status = ERROR;
        }
        else {
          StatusMonitor<T>::status = OK;
        }
        StatusMonitor<T>::status.write();
        group.readAny();
      }
    }
  };

  /** Module for On/off status monitoring.
 * If value monitored is different then desired state (on/off) an error
 * will be reported, otherwise OFF(0) or OK(1) depending on state.
 */
  template<typename T>
  struct StateMonitor : public StatusMonitor<T> {
    using StatusMonitor<T>::StatusMonitor;

    /// The state that we are supposed to have
    ScalarPushInput<T> nominalState{this, "nominalState", "", "", StatusMonitor<T>::_parameterTags};

    /**This is where state evaluation is done*/
    void mainLoop() {
      /** If there is a change either in value monitored or in state, the status is re-evaluated*/
      ReadAnyGroup group{StatusMonitor<T>::oneUp.watch, nominalState};
      while(true) {
        if(StatusMonitor<T>::oneUp.watch != nominalState) {
          StatusMonitor<T>::status = ERROR;
        }
        else if(nominalState == OK || nominalState == OFF) {
          StatusMonitor<T>::status = nominalState;
        }
        else {
          //no correct value
          StatusMonitor<T>::status = ERROR;
        }
        StatusMonitor<T>::status.write();
        group.readAny();
      }
    }
  };
} // namespace ChimeraTK
