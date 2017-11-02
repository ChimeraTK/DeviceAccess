/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "Module.h"
#include "VirtualModule.h"

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
  
  TransferElement::ID Module::readAny() {
    auto accessorList = getAccessorListRecursive();
    // put push-type transfer elements into a list suitable for TransferElement::readAny()
    std::list<std::reference_wrapper<mtca4u::TransferElement>> transferElementList;
    for(auto &accessor : accessorList) {
      if(accessor.getDirection() != VariableDirection::consuming) continue;
      if(accessor.getMode() == UpdateMode::push) {
        transferElementList.emplace_back(accessor.getAppAccessorNoType());
      }
    }
    
    // wait until one of the push-type accessors receives an update
    auto ret = Application::getInstance().readAny(transferElementList);
    
    // trigger read on the poll-type accessors
    for(auto accessor : accessorList) {
      if(accessor.getDirection() != VariableDirection::consuming) continue;
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
      if(accessor.getDirection() != VariableDirection::consuming) continue;
      if(accessor.getMode() != UpdateMode::push) continue;
      accessor.getAppAccessorNoType().read();
    }
    // next non-blockingly read the latest values of all poll-type variables
    for(auto accessor : accessorList) {
      if(accessor.getDirection() != VariableDirection::consuming) continue;
      if(accessor.getMode() == UpdateMode::push) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

/*********************************************************************************************************************/

  void Module::readAllNonBlocking() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      if(accessor.getDirection() != VariableDirection::consuming) continue;
      accessor.getAppAccessorNoType().readNonBlocking();
    }
  }

/*********************************************************************************************************************/

  void Module::readAllLatest() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      if(accessor.getDirection() != VariableDirection::consuming) continue;
      accessor.getAppAccessorNoType().readLatest();
    }
  }

/*********************************************************************************************************************/
  
  void Module::writeAll() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      if(accessor.getDirection() != VariableDirection::feeding) continue;
      accessor.getAppAccessorNoType().write();
    }
  }

} /* namespace ChimeraTK */
