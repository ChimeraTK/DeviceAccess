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

#include "ModuleImpl.h"

namespace ChimeraTK {

  class Application;
  class ModuleGroup;

  class ApplicationModule : public ModuleImpl {

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
      ApplicationModule() {}

      /** Move operation with the move constructor */
      ApplicationModule(ApplicationModule &&other)
      {
        operator=(std::move(other));
      }

      /** Move assignment */
      ApplicationModule& operator=(ApplicationModule &&other) {
        assert(!moduleThread.joinable());   // if the thread is already running, moving is no longer allowed!
        ModuleImpl::operator=(std::move(other));
        return *this;
      }

      /** Destructor */
      virtual ~ApplicationModule();

      /** To be implemented by the user: function called in a separate thread executing the main loop of the module */
      virtual void mainLoop() = 0;

      void run() override;

      void terminate() override;

      ModuleType getModuleType() const override { return ModuleType::ApplicationModule; }

      VersionNumber getCurrentVersionNumber() const override { return currentVersionNumber; }

    protected:

      /** Wrapper around mainLoop(), to execute additional tasks in the thread before entering the main loop */
      void mainLoopWrapper();

      /** The thread executing mainLoop() */
      boost::thread moduleThread;

      void setCurrentVersionNumber(VersionNumber versionNumber) override {
        if(versionNumber > currentVersionNumber) currentVersionNumber = versionNumber;
      }

      /** Version number of last push-type read operation - will be passed on to any write operations */
      VersionNumber currentVersionNumber;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_MODULE_H */
