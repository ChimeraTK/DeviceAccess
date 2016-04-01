/*
 * InstaCoSADev.h
 *
 *  Created on: Mar 29, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4_INSTA_COSA_DEV_H
#define MTCA4_INSTA_COSA_DEV_H

#include <boost/smart_ptr/shared_ptr.hpp>

#include <mtca4u/Device.h>
#include <ControlSystemAdapter/DevicePVManager.h>

namespace mtca4u {

  // forward declaration of the implementation class
  class InstaCoSADev_impl;

  class InstaCoSADev {
    public:

      /** Constructor: pass the process variable manager which shall be used to create the process variables */
      InstaCoSADev(boost::shared_ptr<mtca4u::DevicePVManager> const &processVariableManager);

      /** Create and add accessors and process variables for all registers in the given module. The names of the
       *  process variables will be formed of the pvBaseName which is appended by a slash and then the register name
       *  excluding the given module name. */
      void addModule(mtca4u::Device &device, const mtca4u::RegisterPath &module, const std::string &pvBaseName);

      /** Transfer the data in the given direction. Only the synchronisation between the device register and
       *  the process variable is performed. The synchronisation with the control system has to be triggered
       *  manually. */
      void transferData(mtca4u::SynchronizationDirection direction);


    protected:

      boost::shared_ptr<InstaCoSADev_impl> impl;
  };

} /* namespace mtca4u */

#endif /* MTCA4_INSTA_COSA_DEV_H */
