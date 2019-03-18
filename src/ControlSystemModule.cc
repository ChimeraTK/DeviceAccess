/*
 * ControlSystemModule.cc
 *
 *  Created on: Jun 28, 2016
 *      Author: Martin Hierholzer
 */

#include "ControlSystemModule.h"
#include "Application.h"

namespace ChimeraTK {

  ControlSystemModule::ControlSystemModule() : Module(nullptr, "<ControlSystem>", "") {}

  /*********************************************************************************************************************/

  ControlSystemModule::ControlSystemModule(const std::string& _variableNamePrefix)
  : Module(nullptr,
        _variableNamePrefix.empty() ? "<ControlSystem>" :
                                      _variableNamePrefix.substr(_variableNamePrefix.find_last_of("/") + 1),
        ""),
    variableNamePrefix(_variableNamePrefix) {}

  /*********************************************************************************************************************/

  VariableNetworkNode ControlSystemModule::operator()(
      const std::string& variableName, const std::type_info& valueType, size_t nElements) const {
    assert(variableName.find_first_of("/") == std::string::npos);
    if(variables.count(variableName) == 0) {
      variables[variableName] = {
          variableNamePrefix / variableName, {VariableDirection::invalid, false}, valueType, nElements};
    }
    return variables[variableName];
  }

  /*********************************************************************************************************************/

  Module& ControlSystemModule::operator[](const std::string& moduleName) const {
    assert(moduleName.find_first_of("/") == std::string::npos);
    if(subModules.count(moduleName) == 0) {
      subModules[moduleName] = {variableNamePrefix / moduleName};
    }
    return subModules[moduleName];
  }

  /*********************************************************************************************************************/

  const Module& ControlSystemModule::virtualise() const { return *this; }

  /*********************************************************************************************************************/

  std::list<VariableNetworkNode> ControlSystemModule::getAccessorList() const {
    std::list<VariableNetworkNode> list;
    for(auto& v : variables) list.push_back(v.second);
    return list;
  }

  /*********************************************************************************************************************/

  std::list<Module*> ControlSystemModule::getSubmoduleList() const {
    std::list<Module*> list;
    for(auto& m : subModules) list.push_back(&m.second);
    return list;
  }

  /*********************************************************************************************************************/

} // namespace ChimeraTK
