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
      
      using Module::Module;

      /** Destructor */
      virtual ~ApplicationModule();

      /** To be implemented by the user: function called in a separate thread executing the main loop of the module */
      virtual void mainLoop() = 0;

      void run() override;

      void terminate() override;
      
      VariableNetworkNode operator()(const std::string& variableName) const override;

      Module& operator[](const std::string& moduleName) const override;
      
      /** Move operation with the assignment operator */
      ApplicationModule& operator=(ApplicationModule &&rhs) {
        moduleThread = std::move(rhs.moduleThread);
        _name = std::move(rhs._name);
        _owner = std::move(rhs._owner);
        accessorList = std::move(rhs.accessorList);
        moduleList = std::move(rhs.moduleList);
        return *this;
      }
    
    protected:

      /** Wrapper around mainLoop(), to execute additional tasks in the thread before entering the main loop */
      void mainLoopWrapper();

      /** The thread executing mainLoop() */
      boost::thread moduleThread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_MODULE_H */
