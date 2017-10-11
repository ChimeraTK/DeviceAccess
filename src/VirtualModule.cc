/*
 * VirtualModule.cc
 *
 *  Created on: Apr 4, 2017
 *      Author: Martin Hierholzer
 */

#include <mtca4u/TransferElement.h>

#include "Application.h"
#include "VirtualModule.h"
#include "Module.h"

namespace ChimeraTK {

  VirtualModule::VirtualModule(const VirtualModule &other)
  : Module(nullptr, other.getName(), other.getDescription()) {
    // since moduleList stores plain pointers, we need to regenerate this list
    /// @todo find a better way than storing plain pointers!
    for(auto &mod : other.submodules) addSubModule(mod);
    accessorList = other.accessorList;
    _eliminateHierarchy = other._eliminateHierarchy;
    _moduleType = other.getModuleType();
  }

/*********************************************************************************************************************/

  VirtualModule::~VirtualModule() {
  }

/*********************************************************************************************************************/

  VirtualModule& VirtualModule::operator=(const VirtualModule &other) {
    // move-assign a plain new module
    Module::operator=(VirtualModule(other.getName(), other.getDescription(), other.getModuleType()));
    // since moduleList stores plain pointers, we need to regenerate this list
    /// @todo find a better way than storing plain pointers!
    for(auto &mod : other.submodules) addSubModule(mod);
    accessorList = other.accessorList;
    _eliminateHierarchy = other._eliminateHierarchy;
    return *this;
  }

/*********************************************************************************************************************/
  
  VariableNetworkNode VirtualModule::operator()(const std::string& variableName) const {
    for(auto variable : getAccessorList()) {
      if(variable.getName() == variableName) return VariableNetworkNode(variable);
    }
    throw std::logic_error("Variable '"+variableName+"' is not part of the variable group '"+_name+"'.");
  }

/*********************************************************************************************************************/

  Module& VirtualModule::operator[](const std::string& moduleName) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    throw std::logic_error("Sub-module '"+moduleName+"' is not part of the variable group '"+_name+"'.");
  }

/*********************************************************************************************************************/

  void VirtualModule::connectTo(const Module &target, VariableNetworkNode trigger) const {
    
    // connect all direct variables of this module to their counter-parts in the right-hand-side module
    for(auto variable : getAccessorList()) {
      if(variable.getDirection() == VariableDirection::feeding) {
        variable >> target(variable.getName());
      }
      else {
        // use trigger?
        if(trigger != VariableNetworkNode() && target(variable.getName()).getMode() == UpdateMode::poll
                                            && variable.getMode() == UpdateMode::push ) {
          target(variable.getName()) [ trigger ] >> variable;
        }
        else {
          target(variable.getName()) >> variable;
        }
      }
    }
    
    // connect all sub-modules to their couter-parts in the right-hand-side module
    for(auto submodule : getSubmoduleList()) {
      submodule->connectTo(target[submodule->getName()], trigger);
    }
    
  }

/*********************************************************************************************************************/

  void VirtualModule::addSubModule(VirtualModule module) {
    submodules.push_back(module);
    registerModule(&(submodules.back()));
  }
  
/*********************************************************************************************************************/

  const Module& VirtualModule::virtualise() const {
    return *this;
  }

} /* namespace ChimeraTK */

