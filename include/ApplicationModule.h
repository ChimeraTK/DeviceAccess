/*
 * ApplicationModule.h
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_MODULE_H
#define CHIMERATK_APPLICATION_MODULE_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"

namespace ChimeraTK {

  class ApplicationModule : public Module {

    public:

      /** Destructor */
      virtual ~ApplicationModule();

      /** To be implemented by the user: function called in a separate thread executing the main loop of the module */
      virtual void mainLoop() = 0;

      void run();

      void terminate();

    protected:

      /** Wrapper around mainLoop(), to execute additional tasks in the thread before entering the main loop */
      void mainLoopWrapper();

      /** The thread executing mainLoop() */
      boost::thread moduleThread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_MODULE_H */
