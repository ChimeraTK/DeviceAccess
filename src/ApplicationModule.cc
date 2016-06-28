/*
 * ApplicationModule.cc
 *
 *  Created on: Jun 17, 2016
 *      Author: Martin Hierholzer
 */

#include <thread>

#include "ApplicationModule.h"

namespace ChimeraTK {

  void ApplicationModule::run() {
    std::thread moduleThread(&ApplicationModule::mainLoop, this);
    moduleThread.detach();
  }

} /* namespace ChimeraTK */
