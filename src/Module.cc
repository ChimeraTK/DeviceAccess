/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Module.h"
#include "Application.h"
#include "VirtualModule.h"

namespace ChimeraTK {

  Module::Module(EntityOwner* owner, const std::string& name, const std::string& description,
      HierarchyModifier hierarchyModifier, const std::unordered_set<std::string>& tags)
  : EntityOwner(name, description, hierarchyModifier, tags), _owner(owner) {
    if(_owner != nullptr) _owner->registerModule(this);
  }

  /*********************************************************************************************************************/

  Module::Module(EntityOwner* owner, const std::string& name, const std::string& description, bool eliminateHierarchy,
      const std::unordered_set<std::string>& tags)
  : EntityOwner(name, description, eliminateHierarchy, tags), _owner(owner) {
    if(_owner != nullptr) _owner->registerModule(this);
  }

  /*********************************************************************************************************************/

  Module::~Module() {
    if(_owner != nullptr) _owner->unregisterModule(this);
  }

  /*********************************************************************************************************************/

  Module& Module::operator=(Module&& other) {
    EntityOwner::operator=(std::move(other));
    _owner = other._owner;
    if(_owner != nullptr) _owner->registerModule(this, false);
    // note: the other module unregisters itself in its destructor - will will be
    // called next after any move operation
    return *this;
  }

  /*********************************************************************************************************************/

  ChimeraTK::ReadAnyGroup Module::readAnyGroup() {
    auto recursiveAccessorList = getAccessorListRecursive();

    // put push-type transfer elements into a ReadAnyGroup
    ChimeraTK::ReadAnyGroup group;
    for(auto& accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      group.add(accessor.getAppAccessorNoType());
    }

    group.finalise();
    return group;
  }

  /*********************************************************************************************************************/

  void Module::readAll() {
    auto recursiveAccessorList = getAccessorListRecursive();
    // first blockingly read all push-type variables
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      if(accessor.getMode() != UpdateMode::push) continue;
      accessor.getAppAccessorNoType().read();
    }
    // next non-blockingly read the latest values of all poll-type variables
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      if(accessor.getMode() == UpdateMode::push) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::readAllNonBlocking() {
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      if(accessor.getMode() != UpdateMode::push) continue;
      accessor.getAppAccessorNoType().readNonBlocking();
    }
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      if(accessor.getMode() == UpdateMode::push) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::readAllLatest() {
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::writeAll() {
    auto versionNumber = getCurrentVersionNumber();
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::consuming, false}) continue;
      accessor.getAppAccessorNoType().write(versionNumber);
    }
  }

  /*********************************************************************************************************************/

  void Module::writeAllDestructively() {
    auto versionNumber = getCurrentVersionNumber();
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getDirection() == VariableDirection{VariableDirection::consuming, false}) continue;
      accessor.getAppAccessorNoType().writeDestructively(versionNumber);
    }
  }

  /*********************************************************************************************************************/

  Module& Module::submodule(const std::string& moduleName) const {
    size_t slash = moduleName.find_first_of("/");
    // no slash found: call subscript operator
    if(slash == std::string::npos) return (*this)[moduleName];
    // slash found: split module name at slash
    std::string upperModuleName = moduleName.substr(0, slash);
    std::string remainingModuleNames = moduleName.substr(slash + 1);
    return (*this)[upperModuleName].submodule(remainingModuleNames);
  }

} /* namespace ChimeraTK */
