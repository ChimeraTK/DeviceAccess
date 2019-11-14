/*
 * EntityOwner.cc
 *
 *  Created on: Nov 15, 2016
 *      Author: Martin Hierholzer
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <regex>

#include "EntityOwner.h"
#include "Module.h"
#include "ModuleGraphVisitor.h"
#include "VirtualModule.h"
#include "Application.h"

namespace ChimeraTK {

  EntityOwner::EntityOwner(const std::string& name, const std::string& description, bool eliminateHierarchy,
      const std::unordered_set<std::string>& tags)
  : _name(name), _description(description), _tags(tags) {
    if(eliminateHierarchy) _hierarchyModifier = HierarchyModifier::hideThis;
  }

  /*********************************************************************************************************************/

  EntityOwner::EntityOwner(const std::string& name, const std::string& description, HierarchyModifier hierarchyModifier,
      const std::unordered_set<std::string>& tags)
  : _name(name), _description(description), _hierarchyModifier(hierarchyModifier), _tags(tags) {}

  /*********************************************************************************************************************/

  EntityOwner::~EntityOwner() {}

  /*********************************************************************************************************************/

  EntityOwner& EntityOwner::operator=(EntityOwner&& other) {
    _name = std::move(other._name);
    _description = std::move(other._description);
    accessorList = std::move(other.accessorList);
    moduleList = std::move(other.moduleList);
    _hierarchyModifier = other._hierarchyModifier;
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

  void EntityOwner::registerModule(Module* module, bool addTags) {
    if(addTags)
      for(auto& tag : _tags) module->addTag(tag);
    moduleList.push_back(module);
  }

  /*********************************************************************************************************************/

  void EntityOwner::unregisterModule(Module* module) { moduleList.remove(module); }

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

  VirtualModule EntityOwner::findTag(const std::string& tag) const {
    // create new module to return
    VirtualModule module{_name, _description, getModuleType()};

    // add everything matching the tag to the virtual module and return it
    if(this == &Application::getInstance()) {
      // if this module is the top-level application, we need special treatment for HierarchyModifier::moveToRoot
      findTagAndAppendToModule(module, tag, false, true, false, module);
    }
    else {
      // Not the top-level module: Things that are moved to the top-level are simply discarded
      VirtualModule discard("discarded", "", ModuleType::Invalid);
      findTagAndAppendToModule(module, tag, false, true, false, discard);
    }

    return module;
  }

  /*********************************************************************************************************************/

  VirtualModule EntityOwner::excludeTag(const std::string& tag) const {
    // create new module to return
    VirtualModule module{_name, _description, getModuleType()};

    // add everything matching the tag to the virtual module and return it
    if(this == &Application::getInstance()) {
      // if this module is the top-level application, we need special treatment for HierarchyModifier::moveToRoot
      findTagAndAppendToModule(module, tag, false, true, true, module);
    }
    else {
      // Not the top-level module: Things that are moved to the top-level are simply discarded
      VirtualModule discard("discarded", "", ModuleType::Invalid);
      findTagAndAppendToModule(module, tag, false, true, true, discard);
    }
    return module;
  }

  /*********************************************************************************************************************/

  void EntityOwner::findTagAndAppendToModule(VirtualModule& module, const std::string& tag,
      bool eliminateAllHierarchies, bool eliminateFirstHierarchy, bool negate, VirtualModule& root) const {
    VirtualModule nextmodule{_name, _description, getModuleType()};
    VirtualModule* moduleToAddTo;

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
      for(auto& nodeTag : node.getTags()) {
        if(std::regex_match(nodeTag, expr)) {
          addNode = true;
          break;
        }
      }
      if(node.getTags().size() == 0)
        if(std::regex_match("", expr)) addNode = true; // check if empty tag matches, if no tag applied to node
      if(negate) addNode = !addNode;
      if(addNode) moduleToAddTo->registerAccessor(node);
    }

    // iterate through submodules
    for(auto submodule : getSubmoduleList()) {
      // check if submodule already exists by this name and its hierarchy should
      // not be eliminated
      if(!submodule->getEliminateHierarchy() && moduleToAddTo->hasSubmodule(submodule->getName())) {
        // exists: add to the existing module
        auto* existingSubModule = dynamic_cast<VirtualModule*>(moduleToAddTo->getSubmodule(submodule->getName()));
        assert(existingSubModule != nullptr);
        submodule->findTagAndAppendToModule(*existingSubModule, tag, eliminateAllHierarchies, true, negate, root);
      }
      else {
        // does not yet exist: add as new submodule to the current module
        submodule->findTagAndAppendToModule(*moduleToAddTo, tag, eliminateAllHierarchies, false, negate, root);
      }
    }

    if(needToAddSubModule) {
      if(nextmodule.getAccessorList().size() > 0 || nextmodule.getSubmoduleList().size() > 0) {
        if(_hierarchyModifier == HierarchyModifier::moveToRoot) {
          root.addSubModule(nextmodule);
        }
        else if(_hierarchyModifier == HierarchyModifier::moveToRoot) {
          auto owner = dynamic_cast<VirtualModule*>(module.getOwner());
          if(owner) { // the root does not have an owner.
            owner->addSubModule(nextmodule);
          }
          else {
            throw logic_error(std::string("Module ") + module.getName() +
                ": cannot have hierarchy modifier 'oneLevelUp' in root of the application.");
          }
        }
        else {
          module.addSubModule(nextmodule);
        }
      }
    }
  }

  /*********************************************************************************************************************/

  bool EntityOwner::hasSubmodule(const std::string& name) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == name) return true;
    }
    return false;
  }

  /*********************************************************************************************************************/

  Module* EntityOwner::getSubmodule(const std::string& name) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == name) return submodule;
    }
    throw ChimeraTK::logic_error("Submodule '" + name + "' not found in module '" + getName() + "'!");
  }

  /*********************************************************************************************************************/

  void EntityOwner::dump(const std::string& prefix) const {
    if(prefix == "") {
      std::cout << "==== Hierarchy dump of module '" << _name << "':" << std::endl;
    }

    for(auto& node : getAccessorList()) {
      std::cout << prefix << "+ ";
      node.dump();
    }

    for(auto& submodule : getSubmoduleList()) {
      std::cout << prefix << "| " << submodule->getName() << std::endl;
      submodule->dump(prefix + "| ");
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

  void EntityOwner::addTag(const std::string& tag) {
    for(auto& node : getAccessorList()) node.addTag(tag);
    for(auto& submodule : getSubmoduleList()) submodule->addTag(tag);
    _tags.insert(tag);
  }

  /*********************************************************************************************************************/

  VirtualModule EntityOwner::flatten() {
    VirtualModule nextmodule{_name, _description, getModuleType()};
    for(auto& node : getAccessorListRecursive()) {
      nextmodule.registerAccessor(node);
    }
    return nextmodule;
  }

} /* namespace ChimeraTK */
