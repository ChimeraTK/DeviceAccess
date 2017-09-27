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

namespace ChimeraTK {

  EntityOwner::EntityOwner(const std::string &name, const std::string &description,
                           bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
  : _name(name), _description(description), _eliminateHierarchy(eliminateHierarchy), _tags(tags)
  {}

/*********************************************************************************************************************/

  EntityOwner::~EntityOwner() {}

/*********************************************************************************************************************/

  EntityOwner::EntityOwner(EntityOwner &&other)
  : _name(std::move(other._name)),
    _description(std::move(other._description)),
    accessorList(std::move(other.accessorList)),
    moduleList(std::move(other.moduleList)),
    _eliminateHierarchy(other._eliminateHierarchy),
    _tags(std::move(other._tags))
  {
    for(auto mod : moduleList) {
      mod->setOwner(this);
    }
    for(auto node : accessorList) {
      node.setOwningModule(this);
    }
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

  VirtualModule EntityOwner::findTag(const std::string &tag, bool eliminateAllHierarchies) const {

    // create new module to return
    VirtualModule module{_name, _description, getModuleType()};
    
    // add everything matching the tag to the virtual module and return it
    findTagAndAppendToModule(module, tag, eliminateAllHierarchies, true);
    return module;
  }

  /*********************************************************************************************************************/

  void EntityOwner::findTagAndAppendToModule(VirtualModule &module, const std::string &tag, bool eliminateAllHierarchies,
                                             bool eliminateFirstHierarchy) const {
    
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

  void EntityOwner::dumpGraph(const std::string& fileName) const {
    std::fstream file(fileName, std::ios_base::out);
    file << "digraph G {" << std::endl;
    dumpGraphInternal(file);
    file << "}" << std::endl;
    file.close();
  }

  /*********************************************************************************************************************/

  std::string EntityOwner::cleanDotNode(std::string fullName) const {
    std::replace(fullName.begin(), fullName.end(), '/', '_');
    std::replace(fullName.begin(), fullName.end(), ':', '_');
    return fullName;
  }

  /*********************************************************************************************************************/

  void EntityOwner::dumpGraphInternal(std::ostream &stream) const {
    
    std::string myDotNode = cleanDotNode(getQualifiedName());
    
    stream << myDotNode << "[label=\"" << getName() << "\"";
    if(_eliminateHierarchy) {
      stream << ",style=dotted";
    }
    if(getModuleType() == ModuleType::ModuleGroup) {
      stream << ",peripheries=2";
    }
    if(getModuleType() == ModuleType::ApplicationModule) {
      stream << ",style=bold";
    }
    stream << "]" << std::endl;
    
    for(auto &node : getAccessorList()) {
      std::string dotNode = cleanDotNode(node.getQualifiedName());
      stream << dotNode << "[label=\"{" << node.getName() << "| {";
      bool first = true;
      for(auto tag : node.getTags()) {
        if(!first) {
          stream << "|";
        }
        else {
          first = false;
        }
        stream << tag;
      }
      stream << "}}\", shape=record]" << std::endl;
      stream << "  " << myDotNode << " -> " << dotNode  << std::endl;
    }

    for(auto &submodule : getSubmoduleList()) {
      std::string dotNode = cleanDotNode(submodule->getQualifiedName());
      stream << "  " << myDotNode << " -> " << dotNode << std::endl;
      submodule->dumpGraphInternal(stream);
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
    VirtualModule nextmodule{_name, _description, getModuleType()};
    for(auto &node : getAccessorListRecursive()) {
      nextmodule.registerAccessor(node);
    }
    return nextmodule;
  }
  
} /* namespace ChimeraTK */
