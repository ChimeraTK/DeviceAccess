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
    moduleThread = boost::thread(&ApplicationModule::mainLoop, this);
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

} /* namespace ChimeraTK */
