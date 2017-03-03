/*
 * EntityOwner.cc
 *
 *  Created on: Nov 15, 2016
 *      Author: Martin Hierholzer
 */

#include <cassert>

#include <iostream>

#include "EntityOwner.h"
#include "Module.h"

namespace ChimeraTK {

  EntityOwner::EntityOwner(EntityOwner *owner, const std::string &name)
  : _name(name), _owner(owner)
  {
    if(owner != nullptr) {
      auto thisMustBeAModule = static_cast<Module*>(this);  /// @todo TODO FIXME this is a bit dangerous...
      owner->registerModule(thisMustBeAModule);
    }
  }

/*********************************************************************************************************************/

  EntityOwner::~EntityOwner() {
    if(_owner != nullptr) {
      auto thisMustBeAModule = static_cast<Module*>(this);  /// @todo TODO FIXME this is a bit dangerous...
      _owner->unregisterModule(thisMustBeAModule);
    }
  }

/*********************************************************************************************************************/

  void EntityOwner::registerModule(Module *module) {
    moduleList.push_back(module);
  }

/*********************************************************************************************************************/

  void EntityOwner::unregisterModule(Module *module) {
    moduleList.remove(module);
  }

/*********************************************************************************************************************/

  std::list<VariableNetworkNode> EntityOwner::getAccessorListRecursive() {
    // add accessors of this instance itself
    std::list<VariableNetworkNode> list = getAccessorList();
    
    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      auto sublist = submodule->getAccessorListRecursive();
      list.insert(list.end(), sublist.begin(), sublist.end());
    }
    return list;
  }

/*********************************************************************************************************************/

  std::list<Module*> EntityOwner::getSubmoduleListRecursive() {
    // add modules of this instance itself
    std::list<Module*> list = getSubmoduleList();
    
    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      auto sublist = submodule->getSubmoduleListRecursive();
      list.insert(list.end(), sublist.begin(), sublist.end());
    }
    return list;
  }
  
} /* namespace ChimeraTK */
