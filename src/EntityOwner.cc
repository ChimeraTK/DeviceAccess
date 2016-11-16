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

  void EntityOwner::registerModule(Module *module) {
    moduleList.push_back(module);
  }

/*********************************************************************************************************************/

  const std::list<AccessorBase*> EntityOwner::getAccessorListRecursive() const {
    // add accessors of this instance itself
    std::list<AccessorBase*> list = getAccessorList();
    
    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      auto sublist = submodule->getAccessorListRecursive();
      list.insert(list.end(), sublist.begin(), sublist.end());
    }
    return list;
  }
  
} /* namespace ChimeraTK */
