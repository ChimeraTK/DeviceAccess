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
  
  class Application;
  class ModuleGroup;

  class ApplicationModule : public Module {

    public:
      
      /** Constructor: register the module with its owner. If eliminateHierarchy is true, the hierarchy level
       *  introduced by this module will be eliminated from the "dynamic" data model (see
       *  EntityOwner::setEliminateHierarchy()). The tags given as the last argument are added to all variables
       *  in this module recursively (see EntityOwner::addTag()).
       *
       *  Note: ApplicationModules may only be owned by ModuleGroups or Applications. */
      ApplicationModule(EntityOwner *owner, const std::string &name, const std::string &description,
             bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

      /** Default constructor: Allows late initialisation of modules (e.g. when creating arrays of modules).
       * 
       *  This construtor also has to be here to mitigate a bug in gcc. It is needed to allow constructor
       *  inheritance of modules owning other modules. This constructor will not actually be called then.
       *  See this bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
      ApplicationModule() : Module(nullptr, "invalid", "invalid ApplicationModule") {}

      
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
