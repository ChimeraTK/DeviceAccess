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
    // layer.
    for(auto& variable : getAccessorList()) {
      // We need two loops for this: first we need to read all ConsumingFanOuts (i.e. fan outs triggered by a poll-type
      // read), because they trigger propagation of values to other variables.
      // This is a special case, the standard case is handled in the second loop.
      if(variable.getDirection().dir == VariableDirection::consuming && variable.getMode() == UpdateMode::poll &&
          variable.getOwner().getTriggerType() == VariableNetwork::TriggerType::pollingConsumer) {
        variable.getAppAccessorNoType().read();
      }
    }
    for(auto& variable : getAccessorList()) {
      // In the second loop we deal with all the other variables (this is the standard case)
      if(variable.getDirection().dir == VariableDirection::consuming &&
          (variable.getMode() != UpdateMode::poll ||
              variable.getOwner().getTriggerType() != VariableNetwork::TriggerType::pollingConsumer)) {
        if((variable.getOwner().getFeedingNode().getType() == NodeType::Device ||
               variable.getOwner().getFeedingNode().getType() == NodeType::Constant) &&
            variable.getOwner().getConsumingNodes().size() == 1 &&
            variable.getOwner().getTriggerType() != VariableNetwork::TriggerType::external) {
          // Special case if a push-type device accessor is directly placed into our module (without FanOut): Since the
          // device will not push an initial value to us, a read() would block until the value is changed for the first
          // time. Hence we need to use readLatest() here to obtain the initial value.
          // The same is true for Constant accessors, since their read() never returns.
          variable.getAppAccessorNoType().readLatest();
        }
        else if(variable.getOwner().getFeedingNode().getType() != NodeType::Application) {
          // This case includes inserted FanOuts, which have their own thread, so we must block to make sure to get the
          // initial value. FanOuts will always forward the initial values and the ControlSystemAdapter will always psuh
          // the initial values, hence a blocking read() here will not block indefinitively.
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

} /* namespace ChimeraTK */
