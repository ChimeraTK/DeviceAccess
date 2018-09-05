/*
 * EntityOwner.cc
 *
 *  Created on: Nov 15, 2016
 *      Author: Martin Hierholzer
 */

#include <cassert>
#include <regex>
#include <iostream>
#include <fstream>

#include "EntityOwner.h"
#include "Module.h"
#include "ModuleGraphVisitor.h"
#include "VirtualModule.h"

namespace ChimeraTK {

  EntityOwner::EntityOwner(const std::string &name, const std::string &description,
                           bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
  : _name(name), _description(description), _eliminateHierarchy(eliminateHierarchy), _tags(tags)
  {}

/*********************************************************************************************************************/

  EntityOwner::~EntityOwner() {}

/*********************************************************************************************************************/

  EntityOwner& EntityOwner::operator=(EntityOwner &&other) {
    _name = std::move(other._name);
    _description = std::move(other._description);
    accessorList = std::move(other.accessorList);
    moduleList = std::move(other.moduleList);
    _eliminateHierarchy = other._eliminateHierarchy;
    _tags = std::move(other._tags);
    for(auto mod : moduleList) {
      mod->setOwner(this);
    }
    for(auto node : accessorList) {
      node.setOwningModule(this);
    }
    return *this;
  }

/*********************************************************************************************************************/

  void EntityOwner::registerModule(Module *module, bool addTags) {
    if(addTags) for(auto &tag : _tags) module->addTag(tag);
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

/*********************************************************************************************************************/

  VirtualModule EntityOwner::findTag(const std::string &tag) const {

    // create new module to return
    VirtualModule module{_name, _description, getModuleType()};

    // add everything matching the tag to the virtual module and return it
    findTagAndAppendToModule(module, tag, false, true);
    return module;
  }

/*********************************************************************************************************************/

  VirtualModule EntityOwner::excludeTag(const std::string &tag) const {

    // create new module to return
    VirtualModule module{_name, _description, getModuleType()};

    // add everything matching the tag to the virtual module and return it
    findTagAndAppendToModule(module, tag, false, true, true);
    return module;
  }

/*********************************************************************************************************************/

  void EntityOwner::findTagAndAppendToModule(VirtualModule &module, const std::string &tag, bool eliminateAllHierarchies,
                                             bool eliminateFirstHierarchy, bool negate) const {

    VirtualModule nextmodule{_name, _description, getModuleType()};
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
    std::regex expr(tag);
    for(auto node : getAccessorList()) {
      bool addNode = false;
      for(auto &nodeTag : node.getTags()) {
        if(std::regex_match(nodeTag, expr)) {
          addNode = true;
          break;
        }
      }
      if(node.getTags().size() == 0) if(std::regex_match("", expr)) addNode = true;     // check if empty tag matches, if no tag applied to node
      if(negate) addNode = !addNode;
      if(addNode) moduleToAddTo->registerAccessor(node);
    }

    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      // check if submodule already exists by this name and its hierarchy should not be eliminated
      if(!moduleToAddTo->getEliminateHierarchy() && moduleToAddTo->hasSubmodule(submodule->getName())) {
        // exists: add to the existing module
        auto *existingSubModule = dynamic_cast<VirtualModule*>(moduleToAddTo->getSubmodule(submodule->getName()));
        assert(existingSubModule != nullptr);
        submodule->findTagAndAppendToModule(*existingSubModule, tag, eliminateAllHierarchies, true, negate);
      }
      else {
        // does not yet exist: add as new submodule to the current module
        submodule->findTagAndAppendToModule(*moduleToAddTo, tag, eliminateAllHierarchies, false, negate);
      }
    }

    if(needToAddSubModule) {
      if( nextmodule.getAccessorList().size() > 0 || nextmodule.getSubmoduleList().size() > 0 ) {
        module.addSubModule(nextmodule);
      }
    }

  }

/*********************************************************************************************************************/

  bool EntityOwner::hasSubmodule(const std::string &name) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == name) return true;
    }
    return false;
  }

/*********************************************************************************************************************/

  Module* EntityOwner::getSubmodule(const std::string &name) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == name) return submodule;
    }
    throw; /// @todo make proper exception
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

  void EntityOwner::dumpGraph(const std::string& fileName) const {
    std::fstream file(fileName, std::ios_base::out);
    ModuleGraphVisitor v{file, true};
    v.dispatch(*this);
  }

/*********************************************************************************************************************/

  void EntityOwner::dumpModuleGraph(const std::string& fileName) const {
    std::fstream file(fileName, std::ios_base::out);
    ModuleGraphVisitor v{file, false};
    v.dispatch(*this);
  }

/*********************************************************************************************************************/

  void EntityOwner::addTag(const std::string &tag) {
    for(auto &node : getAccessorList()) node.addTag(tag);
    for(auto &submodule : getSubmoduleList()) submodule->addTag(tag);
    _tags.insert(tag);
  }

/*********************************************************************************************************************/

  VirtualModule EntityOwner::flatten() {
    VirtualModule nextmodule{_name, _description, getModuleType()};
    for(auto &node : getAccessorListRecursive()) {
      nextmodule.registerAccessor(node);
    }
    return nextmodule;
  }

} /* namespace ChimeraTK */
