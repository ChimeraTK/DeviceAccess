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

namespace ChimeraTK {

  /** Base class for internal modules which are created by the variable connection
   * code (e.g. Application::makeConnections()). These modules have to be handled
   * differently since the instance is created dynamically and thus we cannot
   * store the plain pointer in Application::overallModuleList. */
  class InternalModule {
   public:
    virtual ~InternalModule() {}

    /** Activate synchronisation thread if needed */
    virtual void activate() {}

    /** Deactivate synchronisation thread if running*/
    virtual void deactivate() {}
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_INTERNAL_MODULE_H */
