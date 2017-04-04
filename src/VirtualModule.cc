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

  void VirtualModule::addSubModule(const VirtualModule &module) {
    submodules.push_back(module);
    registerModule(&(submodules.back()));
  }
  
} /* namespace ChimeraTK */

