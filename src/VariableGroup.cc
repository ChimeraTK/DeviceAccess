/*
 * VariableGroup.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include <mtca4u/TransferElement.h>

#include "Application.h"
#include "VariableGroup.h"

namespace ChimeraTK {

  VariableGroup::~VariableGroup() {
  }

/*********************************************************************************************************************/
  
  boost::shared_ptr<mtca4u::TransferElement> VariableGroup::readAny() {
    auto accessorList = getAccessorListRecursive();
    // put push-type transfer elements into a list suitable for TransferElement::readAny()
    std::list<std::reference_wrapper<mtca4u::TransferElement>> transferElementList;
    for(auto &accessor : accessorList) {
      if(accessor.getMode() == UpdateMode::push) {
        transferElementList.emplace_back(accessor.getAppAccessorNoType());
      }
    }
    
    // wait until one of the push-type accessors receives an update
    auto ret = Application::getInstance().readAny(transferElementList);
    
    // trigger read on the poll-type accessors
    for(auto accessor : accessorList) {
      if(accessor.getMode() == UpdateMode::poll) {
        accessor.getAppAccessorNoType().readNonBlocking();
      }
    }
    
    return ret;
  }

/*********************************************************************************************************************/
  
  void VariableGroup::readAll() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      accessor.getAppAccessorNoType().read();
    }
  }

/*********************************************************************************************************************/
  
  void VariableGroup::writeAll() {
    auto accessorList = getAccessorListRecursive();
    for(auto accessor : accessorList) {
      accessor.getAppAccessorNoType().write();
    }
  }

/*********************************************************************************************************************/
  
  VariableNetworkNode VariableGroup::operator()(const std::string& variableName) const {
    for(auto variable : getAccessorList()) {
      if(variable.getName() == variableName) return VariableNetworkNode(variable);
    }
    throw std::logic_error("Variable '"+variableName+"' is not part of the variable group '"+_name+"'.");
  }

/*********************************************************************************************************************/

  Module& VariableGroup::operator[](const std::string& moduleName) const {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    throw std::logic_error("Sub-module '"+moduleName+"' is not part of the variable group '"+_name+"'.");
  }

} /* namespace ChimeraTK */

