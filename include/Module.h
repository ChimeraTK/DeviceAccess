/*
 * Module.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_H
#define CHIMERATK_MODULE_H

namespace ChimeraTK {

  /** Base class for ApplicationModule, DeviceModule and ControlSystemModule, to have a common interface for these
   *  module types. */
  class Module {

    public:

      /** Constructor: register the module with the Application */
      Module();

      /** Destructor */
      virtual ~Module() {}

      /** Prepare the execution of the module. */
      virtual void prepare() {};

      /** Execute the module. */
      virtual void run() {};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_H */
