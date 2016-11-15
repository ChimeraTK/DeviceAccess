/*
 * Module.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_H
#define CHIMERATK_MODULE_H

#include "EntityOwner.h"

namespace ChimeraTK {

  /** Base class for ApplicationModule, DeviceModule and ControlSystemModule, to have a common interface for these
   *  module types. */
  class Module : public EntityOwner {

    public:

      /** Constructor: register the module with its owner */
      Module(EntityOwner *owner, const std::string &name);

      /** Destructor */
      virtual ~Module() {}

      /** Prepare the execution of the module. */
      virtual void prepare() {};

      /** Execute the module. */
      virtual void run() {};

      /** Terminate the module. Must be called before destruction, if run() was called previously. */
      virtual void terminate() {};
      
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_H */
