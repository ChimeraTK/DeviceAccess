/*
 * ApplicationModule.h
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_MODULE_H
#define CHIMERATK_APPLICATION_MODULE_H

#include <thread>

#include "Application.h"

namespace ChimeraTK {

  class ApplicationModule {

    public:

      /** Constructor: register the module with the Application */
      ApplicationModule() {
        Application::getInstance().registerModule(*this);
      }

      /** Destructor */
      virtual ~ApplicationModule() {}

      /** To be implemented by the user: function called in a separate thread executing the main loop of the module */
      virtual void mainLoop() = 0;

      /** Execute mainLoop() in a separate thread */
      void run() {
        std::thread moduleThread(&ApplicationModule::mainLoop, this);
        moduleThread.detach();
      }

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_MODULE_H */
