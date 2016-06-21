/*
 * ApplicationModule.cc
 *
 *  Created on: Jun 17, 2016
 *      Author: Martin Hierholzer
 */

#include <thread>

#include "Application.h"
#include "ApplicationModule.h"


namespace ChimeraTK {

  ApplicationModule::ApplicationModule() {
    assert(&(Application::getInstance()) != nullptr);
    Application::getInstance().registerModule(*this);
  }

  void ApplicationModule::run() {
    std::thread moduleThread(&ApplicationModule::mainLoop, this);
    moduleThread.detach();
  }

} /* namespace ChimeraTK */
