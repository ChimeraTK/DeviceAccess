/*
 * ControlSystemModule.h
 *
 *  Created on: Jun 28, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONTROL_SYSTEM_MODULE_H
#define CHIMERATK_CONTROL_SYSTEM_MODULE_H

#include <list>

#include <mtca4u/RegisterPath.h>

#include "VariableNetworkNode.h"
#include "Module.h"

namespace ChimeraTK {

  class ControlSystemModule : public Module {

    public:

      /** Constructor: the optional variableNamePrefix will be prepended to all control system variable names
       *  (separated by a slash). */
      ControlSystemModule(const std::string& variableNamePrefix="");

      /** The function call operator returns a VariableNetworkNode which can be used in the Application::initialise()
       *  function to connect the control system variable with another variable. */
      VariableNetworkNode operator()(const std::string& variableName, const std::type_info &valueType,
                                     size_t nElements=0) const;
      VariableNetworkNode operator()(const std::string& variableName) const override {
        return operator()(variableName, typeid(AnyType));
      }

      Module& operator[](const std::string& moduleName) const override;

    protected:

      mtca4u::RegisterPath variableNamePrefix;
      
      // List of sub modules accessed through the operator[]. This is mutable since it is little more than a cache and
      // thus does not change the logical state of this module
      mutable std::map<std::string, ControlSystemModule> subModules;
      
      // List of variables accessed through the operator(). This is mutable since it is little more than a cache and
      // thus does not change the logical state of this module
      mutable std::map<std::string, VariableNetworkNode> variables;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONTROL_SYSTEM_MODULE_H */
