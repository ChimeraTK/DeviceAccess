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

} /* namespace ChimeraTK */
