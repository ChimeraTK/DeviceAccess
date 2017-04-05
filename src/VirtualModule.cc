/*
 * VirtualModule.cc
 *
 *  Created on: Apr 4, 2017
 *      Author: Martin Hierholzer
 */

#include <mtca4u/TransferElement.h>

#include "Application.h"
#include "VirtualModule.h"

namespace ChimeraTK {

  VirtualModule::VirtualModule(const VirtualModule &other)
  : Module(nullptr, other.getName()) {
    // since moduleList stores plain pointers, we need to regenerate this list
    /// @todo find a better way than storing plain pointers!
    for(auto &mod : other.submodules) addSubModule(mod);
    accessorList = other.accessorList;
    _eliminateHierarchy = other._eliminateHierarchy;
  }

/*********************************************************************************************************************/

  VirtualModule::~VirtualModule() {
  }

/*********************************************************************************************************************/
  
  VariableNetworkNode VirtualModule::operator()(const std::string& variableName) {
    for(auto variable : getAccessorList()) {
      if(variable.getName() == variableName) return VariableNetworkNode(variable);
    }
    throw std::logic_error("Variable '"+variableName+"' is not part of the variable group '"+_name+"'.");
  }

/*********************************************************************************************************************/

  Module& VirtualModule::operator[](const std::string& moduleName) {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    throw std::logic_error("Sub-module '"+moduleName+"' is not part of the variable group '"+_name+"'.");
  }

/*********************************************************************************************************************/

  void VirtualModule::addSubModule(VirtualModule module) {
    submodules.push_back(module);
    registerModule(&(submodules.back()));
  }
  
} /* namespace ChimeraTK */

