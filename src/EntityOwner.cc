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

  bool EntityOwner::getEliminateHierarchy() const {
    return (_hierarchyModifier == HierarchyModifier::hideThis) ||
        (_hierarchyModifier == HierarchyModifier::oneUpAndHide);
  }

  /*********************************************************************************************************************/

  VirtualModule EntityOwner::findTag(const std::string& tag) const {
    // create new module to return
    VirtualModule module{_name, _description, getModuleType()};

    // add everything matching the tag to the virtual module and return it
    if(this == &Application::getInstance()) {
      // if this module is the top-level application, we need special treatment for HierarchyModifier::moveToRoot
      findTagAndAppendToModule(module, tag, false, true, false, module, nullptr);
    }
    else {
      // Not the top-level module: Things that are moved to the top-level are simply discarded
      VirtualModule discard("discarded", "", ModuleType::Invalid);
      findTagAndAppendToModule(module, tag, false, true, false, discard, nullptr);
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
      findTagAndAppendToModule(module, tag, false, true, true, module, nullptr);
    }
    else {
      // Not the top-level module: Things that are moved to the top-level are simply discarded
      VirtualModule discard("discarded", "", ModuleType::Invalid);
      findTagAndAppendToModule(module, tag, false, true, true, discard, nullptr);
    }
    return module;
  }

  /*********************************************************************************************************************/

  // The function adds virtual versions of the EntityOwner itself anf all its children to a virtual module (parent).
  void EntityOwner::findTagAndAppendToModule(VirtualModule& virtualParent, const std::string& tag,
      bool eliminateAllHierarchies, bool eliminateFirstHierarchy, bool negate, VirtualModule& root,
      VirtualModule* virtualGrandparent) const {
    // This function adds the EntityOwner "this" (C++ hierarchy) and its children to the virtual hierarchy.
    /// We must create a virtual version of "this".
    VirtualModule virtualMe{_name, _description, getModuleType()};
    // It might be that it is requested to hide ourselves. In this case we do not add
    // ourselves but directly put the children into the parent (or grand parent, depending on the hierarchy modifier).
    // So we store which module to add to (either virtualMe, the virtual parent or virtual grand parent)
    VirtualModule* moduleToAddTo;
    VirtualModule* parentOfModuleToAddTo;

    bool needToAddMyself = false;
    // Check whether we are supposed to hide. This can be the case if
    // * the hierarchy modifier says so (hideThis or oneUpAndHide -> getEliminateHierarchy is true)
    // * flatten was called (eliminateAllHierarchies is true)
    // * the virtual hierarchy level of virtualMe already exists elsewhere and we just add our children to it
    if(!getEliminateHierarchy() && !eliminateAllHierarchies && !eliminateFirstHierarchy) {
      // We are not hiding ourselves.
      moduleToAddTo = &virtualMe;
      parentOfModuleToAddTo = &virtualParent;
      needToAddMyself = true;
    }
    else {
      // We are hiding. Find the correct ancestor to add to:
      if(_hierarchyModifier == HierarchyModifier::oneUpAndHide) {
        // don't just hide but also move one level up -> add to the grandparent
        moduleToAddTo = virtualGrandparent;
        parentOfModuleToAddTo = nullptr; //FIXME: oneLevelUp and oneUpAndHide cannit be nested
      }
      else {
        // just hide -> add to the parent (could also an already existing objet with the same hierarchy level as virtualMe, where we just add to)
        moduleToAddTo = &virtualParent;
        parentOfModuleToAddTo = virtualGrandparent;
      }
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
    // nomencature: submodule = EntityOwner in C++ hierarchy, child = VirtualModule in virtual hierarcy
    for(auto submodule : getSubmoduleList()) {
      // check if submodule already exists by this name and its hierarchy should
      // not be eliminated
      if(!submodule->getEliminateHierarchy() && moduleToAddTo->hasSubmodule(submodule->getName())) {
        // exists: add to the existing child
        auto* existingChild = dynamic_cast<VirtualModule*>(moduleToAddTo->getSubmodule(submodule->getName()));
        assert(existingChild != nullptr);
        submodule->findTagAndAppendToModule(
            //                                      true = eliminateFirstHierarchy, just add to existingChild
            //                                                                if we add to existingChild, whe have to put to ITS parent here,
            //                                                                which is moduleToAddTo
            *existingChild, tag, eliminateAllHierarchies, true, negate, root, moduleToAddTo);
      }
      else {
        // does not yet exist: add as new submodule to the current module
        // (is also the case if the submodule is hidden and thus not added to the existingChild but the moduleToAddTo)
        submodule->findTagAndAppendToModule(
            *moduleToAddTo, tag, eliminateAllHierarchies, false, negate, root, parentOfModuleToAddTo);
      }
    }

    if(needToAddMyself) {
      if(virtualMe.getAccessorList().size() > 0 || virtualMe.getSubmoduleList().size() > 0) {
        if(_hierarchyModifier == HierarchyModifier::moveToRoot) {
          root.addSubModule(virtualMe);
        }
        else if((_hierarchyModifier == HierarchyModifier::oneLevelUp) ||
            (_hierarchyModifier == HierarchyModifier::oneUpAndHide)) {
          if(virtualGrandparent != nullptr) { // the root does not have a parent. Could be nullptr.
            virtualGrandparent->addSubModule(virtualMe);
          }
          else {
            throw logic_error(std::string("Module ") + virtualParent.getName() +
                ": cannot have hierarchy modifier 'oneLevelUp' or 'oneUpAndHide' in root of the application." +
                "\nNon-virtual path to the offending module: " + getQualifiedName());
          }
        }
        else {
          virtualParent.addSubModule(virtualMe);
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

  bool EntityOwner::hasReachedTestableMode() { return testableModeReached; }

} /* namespace ChimeraTK */
