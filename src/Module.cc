/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "Module.h"

namespace ChimeraTK {

  Module::Module(EntityOwner *owner, const std::string &name, const std::string &description,
                 bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
  : EntityOwner(name, description, eliminateHierarchy, tags),
    _owner(owner)
  {
    if(_owner != nullptr) _owner->registerModule(this);
  }
  
/*********************************************************************************************************************/

  Module::~Module()
  {
    if(_owner != nullptr) _owner->unregisterModule(this);
  }

/*********************************************************************************************************************/
  
  boost::shared_ptr<mtca4u::TransferElement> Module::readAny() {
    auto accessorList = getAccessorListRecursive();
    // put push-type transfer elements into a list suitable for TransferElement::readAny()
    std::list<std::reference_wrapper<mtca4u::TransferElement>> transferElementList;
    for(auto &accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isReadable()) continue;
      if(accessor.getMode() == UpdateMode::push) {
        transferElementList.emplace_back(accessor.getAppAccessorNoType());
      }
    }
    
    // wait until one of the push-type accessors receives an update
    auto ret = Application::getInstance().readAny(transferElementList);
    
    // trigger read on the poll-type accessors
    for(auto accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isReadable()) continue;
      if(accessor.getMode() == UpdateMode::poll) {
        accessor.getAppAccessorNoType().readLatest();
      }
    }
    
    return ret;
  }

/*********************************************************************************************************************/
  
  void Module::readAll() {
    auto accessorList = getAccessorListRecursive();
    // first blockingly read all push-type variables
    for(auto accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isReadable()) continue;
      if(accessor.getMode() != UpdateMode::push) continue;
      accessor.getAppAccessorNoType().read();
    }
    // next non-blockingly read the latest values of all poll-type variables
    for(auto accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isReadable()) continue;
      if(accessor.getMode() == UpdateMode::push) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

/*********************************************************************************************************************/

  void Module::readAllNonBlocking() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isReadable()) continue;
      accessor.getAppAccessorNoType().readNonBlocking();
    }
  }

/*********************************************************************************************************************/

  void Module::readAllLatest() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isReadable()) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

/*********************************************************************************************************************/
  
  void Module::writeAll() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      if(!accessor.getAppAccessorNoType().isWriteable()) continue;
      accessor.getAppAccessorNoType().write();
    }
  }

/*********************************************************************************************************************/
  
  VariableNetworkNode Module::operator()(const std::string& variableName) const {
    // search for variable in our list
    for(auto variable : getAccessorList()) {
      if(variable.getName() == variableName) return VariableNetworkNode(variable);
    }
    // recurse into those submodules which have the eliminate hierarchy flag on
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getEliminateHierarchy()) {
        try {
          return (*submodule)(variableName);
        }
        catch(std::logic_error &e) {
          // ignore this exception, it just means this submodule does not have this variable
          /// @todo FIXME don't use exceptions for regular program flow, add a function to test the presence of a variable instead!
        }
      }
    }
    // variable not found anywhere: throw exception
    throw std::logic_error("Variable '"+variableName+"' is not part of the module '"+_name+"'.");
  }

/*********************************************************************************************************************/

  Module& Module::operator[](const std::string& moduleName) const {
    // search for module in our list
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    // recurse into those submodules which have the eliminate hierarchy flag on
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getEliminateHierarchy()) {
        try {
          return (*submodule)[moduleName];
        }
        catch(std::logic_error &e) {
          // ignore this exception, it just means this submodule does not have this module
          /// @todo FIXME don't use exceptions for regular program flow, add a function to test the presence of a module instead!
        }
      }
    }
    // module not found anywhere: throw exception
    throw std::logic_error("Sub-module '"+moduleName+"' is not part of the module '"+_name+"'.");
  }

} /* namespace ChimeraTK */
