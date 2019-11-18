/*
 * VirtualModule.cc
 *
 *  Created on: Apr 4, 2017
 *      Author: Martin Hierholzer
 */

#include <ChimeraTK/TransferElement.h>

#include "Application.h"
#include "Module.h"
#include "VirtualModule.h"

namespace ChimeraTK {

  VirtualModule::VirtualModule(const VirtualModule& other) : Module(nullptr, other.getName(), other.getDescription()) {
    // since moduleList stores plain pointers, we need to regenerate this list
    /// @todo find a better way than storing plain pointers!
    for(auto& mod : other.submodules) addSubModule(mod);
    accessorList = other.accessorList;
    _hierarchyModifier = other._hierarchyModifier;
    _moduleType = other.getModuleType();
  }

  /*********************************************************************************************************************/

  VirtualModule::~VirtualModule() {}

  /*********************************************************************************************************************/

  VirtualModule& VirtualModule::operator=(const VirtualModule& other) {
    // move-assign a plain new module
    Module::operator=(VirtualModule(other.getName(), other.getDescription(), other.getModuleType()));
    // since moduleList stores plain pointers, we need to regenerate this list
    /// @todo find a better way than storing plain pointers!
    for(auto& mod : other.submodules) addSubModule(mod);
    accessorList = other.accessorList;
    _hierarchyModifier = other._hierarchyModifier;
    return *this;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode VirtualModule::operator()(const std::string& variableName) const {
    for(auto variable : getAccessorList()) {
      if(variable.getName() == variableName) return VariableNetworkNode(variable);
    }
    throw ChimeraTK::logic_error("Variable '" + variableName + "' is not part of the variable group '" + _name + "'.");
  }

  /*********************************************************************************************************************/

  Module& VirtualModule::operator[](const std::string& moduleName) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    throw ChimeraTK::logic_error("Sub-module '" + moduleName + "' is not part of the variable group '" + _name + "'.");
  }

  /*********************************************************************************************************************/

  void VirtualModule::connectTo(const Module& target, VariableNetworkNode trigger) const {
    // connect all direct variables of this module to their counter-parts in the
    // right-hand-side module
    for(auto variable : getAccessorList()) {
      if(variable.getDirection().dir == VariableDirection::feeding) {
        // use trigger?
        if(trigger != VariableNetworkNode() && target(variable.getName()).getMode() == UpdateMode::push &&
            variable.getMode() == UpdateMode::poll) {
          variable[trigger] >> target(variable.getName());
        }
        else {
          variable >> target(variable.getName());
        }
      }
      else {
        // use trigger?
        if(trigger != VariableNetworkNode() && target(variable.getName()).getMode() == UpdateMode::poll &&
            variable.getMode() == UpdateMode::push) {
          target(variable.getName())[trigger] >> variable;
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

  void VirtualModule::addAccessor(VariableNetworkNode accessor) { accessorList.push_back(accessor); }

  /*********************************************************************************************************************/

  void VirtualModule::addSubModule(VirtualModule module) {
    if(!hasSubmodule(module.getName())) {
      // Submodule doesn'st exist already: register the given module as a new submodule
      submodules.push_back(module);
      registerModule(&(submodules.back()));
    }
    else {
      // Submodule does exist already: copy content into the existing submodule
      VirtualModule& theSubmodule = dynamic_cast<VirtualModule&>(submodule(module.getName()));
      for(auto& submod : module.getSubmoduleList()) {
        theSubmodule.addSubModule(dynamic_cast<VirtualModule&>(*submod));
      }
      for(auto& acc : module.getAccessorList()) {
        theSubmodule.addAccessor(acc);
      }
    }
  }

  /*********************************************************************************************************************/

  const Module& VirtualModule::virtualise() const { return *this; }

  /*********************************************************************************************************************/

  VirtualModule& VirtualModule::createAndGetSubmodule(const RegisterPath& moduleName) {
    for(auto& sm : submodules) {
      if(moduleName == sm.getName()) return sm;
    }
    addSubModule(VirtualModule(std::string(moduleName).substr(1), getDescription(), getModuleType()));
    return submodules.back();
  }

  /*********************************************************************************************************************/

  VirtualModule& VirtualModule::createAndGetSubmoduleRecursive(const RegisterPath& moduleName) {
    auto slash = std::string(moduleName).find_first_of("/", 1);
    if(slash == std::string::npos) {
      return createAndGetSubmodule(moduleName);
    }
    else {
      auto firstSubmodule = std::string(moduleName).substr(0, slash);
      auto remainingSubmodules = std::string(moduleName).substr(slash + 1);
      return createAndGetSubmodule(firstSubmodule).createAndGetSubmoduleRecursive(remainingSubmodules);
    }
  }

  /*********************************************************************************************************************/

} /* namespace ChimeraTK */
