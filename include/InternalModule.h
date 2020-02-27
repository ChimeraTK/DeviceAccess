/*
 * InternalModule.h
 *
 *  Created on: Jun 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_INTERNAL_MODULE_H
#define CHIMERATK_INTERNAL_MODULE_H

#include <thread>

#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>

#include "EntityOwner.h"

namespace ChimeraTK {

  /** Base class for internal modules which are created by the variable connection code (e.g.
   *  Application::makeConnections()). These modules have to be handled  differently since the instance is created
   *  dynamically and thus we cannot store the plain pointer in Application::overallModuleList.
   * 
   *  @todo Currently this class is based on EntityOwner somewhat artificially. Instead the InternalModule class needs
   *  to be properly unified with the normal Module classes. */
  class InternalModule : public EntityOwner {
   public:
    ~InternalModule() override {}

    /** Activate synchronisation thread if needed
     *  @todo: Unify with Module::run() */
    virtual void activate() {}

    /** Deactivate synchronisation thread if running
     *  @todo: Unify with Module::terminate() */
    virtual void deactivate() {}

    /** Below all pure virtual functions of EntityOwner are "implemented" just to make the program compile for now.
     *  They are currently not used. */
    std::string getQualifiedName() const override { throw; }
    std::string getFullDescription() const override { throw; }
    ModuleType getModuleType() const override { throw; }
    VersionNumber getCurrentVersionNumber() const override { throw; }
    void setCurrentVersionNumber(VersionNumber /*versionNumber*/) override { throw; }
    DataValidity getDataValidity() const override { throw; }
    void incrementDataFaultCounter() override { throw; }
    void decrementDataFaultCounter() override { throw; }
    void incrementExceptionCounter() override { throw; }
    void decrementExceptionCounter() override { throw; }
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_INTERNAL_MODULE_H */
