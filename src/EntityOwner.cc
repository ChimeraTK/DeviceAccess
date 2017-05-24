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
#include "VirtualModule.h"
#include "VariableGroup.h"

namespace ChimeraTK {

  EntityOwner::EntityOwner(EntityOwner *owner, const std::string &name, const std::string &description,
                           bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
  : _name(name), _description(description), _owner(owner), _eliminateHierarchy(eliminateHierarchy), _tags(tags)
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
    for(auto &tag : _tags) module->addTag(tag);
    moduleList.push_back(module);
  }

/*********************************************************************************************************************/

  void EntityOwner::unregisterModule(Module *module) {
    moduleList.remove(module);
  }

/*********************************************************************************************************************/

  std::list<VariableNetworkNode> EntityOwner::getAccessorListRecursive(bool includeSubmodules) {
    // add accessors of this instance itself
    std::list<VariableNetworkNode> list = getAccessorList();
    
    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      // ignore anything that is not a VariableGroup if submodules should not be included
      /// @todo Add test for this!
      if(!includeSubmodules && dynamic_cast<VariableGroup*>(submodule) == nullptr) continue;
      // obtail list of accessors from the submodule/group and insert it into the list
      auto sublist = submodule->getAccessorListRecursive(includeSubmodules);
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

/*********************************************************************************************************************/

  VirtualModule EntityOwner::findTag(const std::string &tag, bool eliminateAllHierarchies) const {

    // create new module to return
    VirtualModule module{_name, _description};
    
    // add everything matching the tag to the virtual module and return it
    findTagAndAppendToModule(module, tag, eliminateAllHierarchies, true);
    return module;
  }

  /*********************************************************************************************************************/

  void EntityOwner::findTagAndAppendToModule(VirtualModule &module, const std::string &tag, bool eliminateAllHierarchies,
                                             bool eliminateFirstHierarchy) const {
    
    VirtualModule nextmodule{_name, _description};
    VirtualModule *moduleToAddTo;
    
    bool needToAddSubModule = false;
    if(!getEliminateHierarchy() && !eliminateAllHierarchies && !eliminateFirstHierarchy) {
      moduleToAddTo = &nextmodule;
      needToAddSubModule = true;
    }
    else {
      moduleToAddTo = &module;
    }
    
    // add nodes to the module if matching the tag
    for(auto node : getAccessorList()) {
      if(node.getTags().count(tag) > 0) {
        moduleToAddTo->registerAccessor(node);
      }
    }

    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      submodule->findTagAndAppendToModule(*moduleToAddTo, tag, eliminateAllHierarchies);
    }
    
    if(needToAddSubModule) {
      if( nextmodule.getAccessorList().size() > 0 || nextmodule.getSubmoduleList().size() > 0 ) {
        module.addSubModule(nextmodule);
      }
    }
    
  }

  /*********************************************************************************************************************/

  void EntityOwner::dump(const std::string &prefix) const {
    
    if(prefix == "") {
      std::cout << "==== Hierarchy dump of module '" << _name << "':" << std::endl;
    }
    
    for(auto &node : getAccessorList()) {
      std::cout << prefix << "+ ";
      node.dump();
    }

    for(auto &submodule : getSubmoduleList()) {
      std::cout << prefix << "| " << submodule->getName() << std::endl;
      submodule->dump(prefix+"| ");
    }
    
  }

  /*********************************************************************************************************************/

  void EntityOwner::addTag(const std::string &tag) {
    for(auto &node : getAccessorList()) node.addTag(tag);
    for(auto &submodule : getSubmoduleList()) submodule->addTag(tag);
    _tags.insert(tag);
  }

  /*********************************************************************************************************************/

  VirtualModule EntityOwner::flatten() {
    VirtualModule nextmodule{_name, _description};
    for(auto &node : getAccessorListRecursive(true)) {
      nextmodule.registerAccessor(node);
    }
    return nextmodule;
  }
  
} /* namespace ChimeraTK */
