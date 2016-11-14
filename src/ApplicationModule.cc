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

    // start the module thread
    assert(!moduleThread.joinable());
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
