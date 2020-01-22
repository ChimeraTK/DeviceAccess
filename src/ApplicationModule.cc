/*
 * ApplicationModule.cc
 *
 *  Created on: Jun 17, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationCore.h"
#include "ConfigReader.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  ApplicationModule::ApplicationModule(EntityOwner* owner, const std::string& name, const std::string& description,
      HierarchyModifier hierarchyModifier, const std::unordered_set<std::string>& tags)
  : ModuleImpl(owner, name, description, hierarchyModifier, tags) {
    if(!dynamic_cast<ModuleGroup*>(owner) && !dynamic_cast<Application*>(owner)) {
      throw ChimeraTK::logic_error("ApplicationModules must be owned either by ModuleGroups or the Application!");
    }
    if(name.find_first_of("/") != std::string::npos) {
      throw ChimeraTK::logic_error(
          "Module names must not contain slashes: '" + name + " owned by '" + owner->getQualifiedName() + "'.");
    }
  }

  /*********************************************************************************************************************/

  ApplicationModule::ApplicationModule(EntityOwner* owner, const std::string& name, const std::string& description,
      bool eliminateHierarchy, const std::unordered_set<std::string>& tags)
  : ModuleImpl(owner, name, description, eliminateHierarchy, tags) {
    if(!dynamic_cast<ModuleGroup*>(owner) && !dynamic_cast<Application*>(owner)) {
      throw ChimeraTK::logic_error("ApplicationModules must be owned either by ModuleGroups or the Application!");
    }
    if(name.find_first_of("/") != std::string::npos) {
      throw ChimeraTK::logic_error(
          "Module names must not contain slashes: '" + name + " owned by '" + owner->getQualifiedName() + "'.");
    }
  }

  /*********************************************************************************************************************/

  void ApplicationModule::run() {
    // start the module thread
    assert(!moduleThread.joinable());
    moduleThread = boost::thread(&ApplicationModule::mainLoopWrapper, this);
    //Wait until the thread has launched and acquired and released the testable mode lock at least once.
    if(Application::getInstance().isTestableModeEnabled()) {
      while(!testableModeReached) {
        Application::getInstance().testableModeUnlock("releaseForReachTestableMode");
        usleep(100);
        Application::getInstance().testableModeLock("acquireForReachTestableMode");
      }
    }
  }

  /*********************************************************************************************************************/

  void ApplicationModule::terminate() {
    if(moduleThread.joinable()) {
      moduleThread.interrupt();
      // try joining the thread
      while(!moduleThread.try_join_for(boost::chrono::milliseconds(10))) {
        // if thread is not yet joined, send interrupt() to all variables.
        for(auto& var : getAccessorListRecursive()) {
          var.getAppAccessorNoType().getHighLevelImplElement()->interrupt();
        }
        // it may not suffice to send interrupt() once, as the exception might get
        // overwritten in the queue, thus we repeat this until the thread was
        // joined.
      }
    }
    assert(!moduleThread.joinable());
  }

  /*********************************************************************************************************************/

  ApplicationModule::~ApplicationModule() { assert(!moduleThread.joinable()); }

  /*********************************************************************************************************************/

  void ApplicationModule::mainLoopWrapper() {
    Application::registerThread("AM_" + getName());

    // Acquire testable mode lock, so from this point on we are running only one user thread concurrently
    Application::testableModeLock("start");

    // Read all variables once to obtain the initial values from the devices and from the control system persistency
    // layer. This is done in two steps, first for all poll-type variables and then for all push-types, because
    // poll-type reads might trigger distribution of values to push-type variables via a ConsumingFanOut.
    for(auto& variable : getAccessorListRecursive()) {
      if(variable.getDirection().dir == VariableDirection::feeding) continue;
      if(variable.getMode() == UpdateMode::poll) {
        auto hasInitialValue = variable.hasInitialValue();
        if(hasInitialValue != VariableNetworkNode::InitialValueMode::None) {
          variable.getAppAccessorNoType().readLatest();
        }
      }
    }
    for(auto& variable : getAccessorListRecursive()) {
      if(variable.getDirection().dir == VariableDirection::feeding) continue;
      if(variable.getMode() == UpdateMode::push) {
        auto hasInitialValue = variable.hasInitialValue();
        if(hasInitialValue == VariableNetworkNode::InitialValueMode::Poll) {
          variable.getAppAccessorNoType().readLatest();
        }
        else if(hasInitialValue == VariableNetworkNode::InitialValueMode::Push) {
          variable.getAppAccessorNoType().read();
        }
      }
    }

    // We are holding the testable mode lock, so we are sure the mechanism will work now.
    testableModeReached = true;

    // enter the main loop
    mainLoop();
    Application::testableModeUnlock("terminate");
  }

  void ApplicationModule::incrementDataFaultCounter(bool writeAllOutputs) {
    ++faultCounter;
    // writeAll only once for first incrementDataFaultCounter call -> going with faultCounter from 0 to 1
    if (writeAllOutputs && faultCounter == 1)
      this->writeAll(); 
  }

  void ApplicationModule::decrementDataFaultCounter(bool writeAllOutputs) {
    assert(faultCounter > 0);
    --faultCounter;
    //writeAll only once for last decrementDataFaultCounter call -> going with faultCounter from 1 to 0
    if (writeAllOutputs && faultCounter == 0)
      this->writeAll();
  }
  
  
} /* namespace ChimeraTK */
