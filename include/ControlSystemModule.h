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
                                     size_t nElements=0);
      VariableNetworkNode operator()(const std::string& variableName) {
        return operator()(variableName, typeid(AnyType));
      }

      Module& operator[](const std::string& moduleName) {
        subModules.emplace_back(variableNamePrefix/moduleName);
        return subModules.back();
      }

    protected:

      mtca4u::RegisterPath variableNamePrefix;
      
      std::list<ControlSystemModule> subModules;
      
      std::map<std::string, VariableNetworkNode> variables;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONTROL_SYSTEM_MODULE_H */
