/*
 * ControlSystemModule.cc
 *
 *  Created on: Jun 28, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "ControlSystemModule.h"

namespace ChimeraTK {

  ControlSystemModule::ControlSystemModule(const std::string& _variableNamePrefix)
  : Module(&(Application::getInstance()), "ControlSystem:"+_variableNamePrefix, ""),
    variableNamePrefix(_variableNamePrefix)
  {}

  /*********************************************************************************************************************/

  VariableNetworkNode ControlSystemModule::operator()(const std::string& variableName, const std::type_info &valueType,
                                                      size_t nElements) const {
    if(variables.count(variableName) == 0) {
      variables[variableName] = {variableNamePrefix/variableName, VariableDirection::invalid, valueType, nElements};
    }
    return variables[variableName];
  }

  /*********************************************************************************************************************/

  Module& ControlSystemModule::operator[](const std::string& moduleName) const {
    if(subModules.count(moduleName) == 0) {
      subModules[moduleName] = {variableNamePrefix/moduleName};
    }
    return subModules[moduleName];
  }

}
