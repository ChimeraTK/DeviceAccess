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
#include <mtca4u/ForwardDeclarations.h>

namespace ChimeraTK {

  class DeviceModule : public Module {

    public:

      /** Constructor: The device represented by this DeviceModule is identified by either the device alias found
       *  in the DMAP file or directly an URI. The given optional prefix will be prepended to all register names
       *  (separated by a slash). */
      DeviceModule(const std::string& deviceAliasOrURI, const std::string& registerNamePrefix="");

      /** The subscript operator returns a VariableNetworkNode which can be used in the Application::initialise()
       *  function to connect the register with another variable. */
      VariableNetworkNode operator()(const std::string& registerName, UpdateMode mode,
          const std::type_info &valueType=typeid(AnyType), size_t nElements=0);
      VariableNetworkNode operator()(const std::string& registerName, const std::type_info &valueType,
          size_t nElements=0, UpdateMode mode=UpdateMode::poll) {
        return operator()(registerName, mode, valueType, nElements);
      }
      VariableNetworkNode operator()(const std::string& variableName) {
        return operator()(variableName, UpdateMode::poll);
      }

      Module& operator[](const std::string& moduleName) {
        subModules.emplace_back(deviceAliasOrURI, registerNamePrefix/moduleName);
        return subModules.back();
      }

      /** Prepare the device for usage (i.e. open it) */
      void prepare();

    protected:

      /** The backend used to access this device */
      boost::shared_ptr<mtca4u::DeviceBackend> backend;

      std::string deviceAliasOrURI;
      mtca4u::RegisterPath registerNamePrefix;
      
      std::list<DeviceModule> subModules;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_MODULE_H */
