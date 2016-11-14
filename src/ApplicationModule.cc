/*
 * ApplicationModule.cc
 *
 *  Created on: Jun 17, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationModule.h"
#include "Accessor.h"

namespace ChimeraTK {

  void ApplicationModule::run() {
    assert(!moduleThread.joinable());

    // read all control system to application variables once, to set the startup value e.g. coming from the config file
    // (without triggering an action inside the application)
    for(auto &variable : getAccessorList()) {
      if( variable->getNode().getOwner().getFeedingNode().getType() == NodeType::ControlSystem ) {
        variable->readNonBlocking();
      }
    }

    moduleThread = boost::thread(&ApplicationModule::mainLoopWrapper, this);
  }

  void ApplicationModule::terminate() {
    if(moduleThread.joinable()) {
      moduleThread.interrupt();
      moduleThread.join();
    }
    assert(!moduleThread.joinable());
  }

  ApplicationModule::~ApplicationModule() {
    assert(!moduleThread.joinable());
  }
  
  void ApplicationModule::mainLoopWrapper() {
    // enter the main loop
    mainLoop();
  }

} /* namespace ChimeraTK */
