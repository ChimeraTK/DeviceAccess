/*
 * DeviceModule.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEVICE_MODULE_H
#define CHIMERATK_DEVICE_MODULE_H

#include "VariableNetworkNode.h"
#include "Module.h"

namespace mtca4u {
  class DeviceBackend;
}

namespace ChimeraTK {

  class DeviceModule : public Module {

    public:

      /** Constructor: The device represented by this DeviceModule is identified by either the device alias found
       *  in the DMAP file or directly an URI. The given optional prefix will be prepended to all register names. */
      DeviceModule(const std::string& deviceAliasOrURI, const std::string& registerNamePrefix="");

      /** The subscript operator returns a VariableNetworkNode which can be used in the Application::initialise()
       *  function to connect the register with another variable. */
      VariableNetworkNode operator()(const std::string& registerName, UpdateMode mode=UpdateMode::poll,
          const std::type_info &valueType=typeid(AnyType));
      VariableNetworkNode operator()(const std::string& registerName, const std::type_info &valueType,
          UpdateMode mode=UpdateMode::poll) {
        return operator()(registerName, mode, valueType);
      }

      /** Prepare the device for usage (i.e. open it) */
      void prepare();

    protected:

      /** The backend used to access this device */
      boost::shared_ptr<mtca4u::DeviceBackend> backend;

      std::string deviceAliasOrURI;
      mtca4u::RegisterPath registerNamePrefix;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_MODULE_H */
