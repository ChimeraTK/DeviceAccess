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

  void Module::readAll(bool includeReturnChannels) {
    auto recursiveAccessorList = getAccessorListRecursive();
    // first blockingly read all push-type variables
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() != UpdateMode::push) continue;
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      }
      accessor.getAppAccessorNoType().read();
    }
    // next non-blockingly read the latest values of all poll-type variables
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() == UpdateMode::push) continue;
      // poll-type accessors cannot have a readback channel
      if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::readAllNonBlocking(bool includeReturnChannels) {
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() != UpdateMode::push) continue;
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      }
      accessor.getAppAccessorNoType().readNonBlocking();
    }
    for(auto accessor : recursiveAccessorList) {
      if(accessor.getMode() == UpdateMode::push) continue;
      // poll-type accessors cannot have a readback channel
      if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::readAllLatest(bool includeReturnChannels) {
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::feeding, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::consuming) continue;
      }
      accessor.getAppAccessorNoType().readLatest();
    }
  }

  /*********************************************************************************************************************/

  void Module::writeAll(bool includeReturnChannels) {
    auto versionNumber = getCurrentVersionNumber();
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::consuming, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::feeding) continue;
      }
      accessor.getAppAccessorNoType().write(versionNumber);
    }
  }

  /*********************************************************************************************************************/

  void Module::writeAllDestructively(bool includeReturnChannels) {
    auto versionNumber = getCurrentVersionNumber();
    auto recursiveAccessorList = getAccessorListRecursive();
    for(auto accessor : recursiveAccessorList) {
      if(includeReturnChannels) {
        if(accessor.getDirection() == VariableDirection{VariableDirection::consuming, false}) continue;
      }
      else {
        if(accessor.getDirection().dir != VariableDirection::feeding) continue;
      }
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

  std::string Module::getVirtualQualifiedName() const {
    std::string virtualQualifiedName{""};
    const EntityOwner* currentLevelModule{this};

    bool rootReached{false};
    do {

      if(currentLevelModule == &Application::getInstance()) {
        rootReached = true;
      }

      auto currentLevelModifier = currentLevelModule->getHierarchyModifier();

      switch(currentLevelModifier) {
        case HierarchyModifier::none:
          virtualQualifiedName = "/" + currentLevelModule->getName() + virtualQualifiedName;
          break;
        case HierarchyModifier::hideThis:
          // Omit name of current level
          break;
        case HierarchyModifier::oneLevelUp:
          virtualQualifiedName = "/" + currentLevelModule->getName() + virtualQualifiedName;
          // Need to leave out to next level
          // TODO This needs to catch the case that the mdifier is illegally used on the first level
          currentLevelModule = dynamic_cast<const Module*>(currentLevelModule)->getOwner();
          break;
        case HierarchyModifier::oneUpAndHide:
          // Need to leave out to next level
          currentLevelModule = dynamic_cast<const Module*>(currentLevelModule)->getOwner();
          break;
        case HierarchyModifier::moveToRoot:
          virtualQualifiedName =
              "/" + Application::getInstance().getName() + "/" + currentLevelModule->getName() + virtualQualifiedName;
          rootReached = true;
      }

      if(!rootReached) {
        currentLevelModule = dynamic_cast<const Module*>(currentLevelModule)->getOwner();
      }
    } while(!rootReached);

    //    std::string fullName = getName();
    //    fullName  += ", virtual module of owner: " + getOwner()->findTag(".*").getName();
    //    return fullName;
    return virtualQualifiedName;
  }

} /* namespace ChimeraTK */
