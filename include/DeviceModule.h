/*
 * DeviceModule.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEVICE_MODULE_H
#define CHIMERATK_DEVICE_MODULE_H

#include <mtca4u/ForwardDeclarations.h>
#include <mtca4u/RegisterPath.h>

#include "VariableNetworkNode.h"
#include "Module.h"

namespace ChimeraTK {

  class DeviceModule : public Module {

    public:

      /** Constructor: The device represented by this DeviceModule is identified by either the device alias found
       *  in the DMAP file or directly an URI. The given optional prefix will be prepended to all register names
       *  (separated by a slash). */
      DeviceModule(const std::string& deviceAliasOrURI, const std::string& registerNamePrefix="");

      /** Default constructor: create dysfunctional device module */
      DeviceModule() {}

      /** The subscript operator returns a VariableNetworkNode which can be used in the Application::initialise()
       *  function to connect the register with another variable. */
      VariableNetworkNode operator()(const std::string& registerName, UpdateMode mode,
          const std::type_info &valueType=typeid(AnyType), size_t nElements=0) const;
      VariableNetworkNode operator()(const std::string& registerName, const std::type_info &valueType,
          size_t nElements=0, UpdateMode mode=UpdateMode::poll) const {
        return operator()(registerName, mode, valueType, nElements);
      }
      VariableNetworkNode operator()(const std::string& variableName) const override {
        return operator()(variableName, UpdateMode::poll);
      }

      Module& operator[](const std::string& moduleName) const override;

    protected:

      std::string deviceAliasOrURI;
      mtca4u::RegisterPath registerNamePrefix;
      
      // List of sub modules accessed through the operator[]. This is mutable since it is little more than a cache and
      // thus does not change the logical state of this module
      mutable std::map<std::string, DeviceModule> subModules;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_MODULE_H */
