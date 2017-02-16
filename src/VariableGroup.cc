/*
 * VariableGroup.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include <mtca4u/TransferElement.h>

#include "VariableGroup.h"

namespace ChimeraTK {

  VariableGroup::~VariableGroup() {
  }

/*********************************************************************************************************************/
  
  void VariableGroup::readAny() {
    auto accessorList = getAccessorListRecursive();

    // put push-type transfer elements into a list suitable for TransferElement::readAny()
    std::list<std::reference_wrapper<mtca4u::TransferElement>> transferElementList;
    for(auto &accessor : accessorList) {
      if(accessor.getMode() == UpdateMode::push) {
        transferElementList.emplace_back(accessor.getAppAccessorNoType());
      }
    }
    
    // wait until one of the push-type accessors receives an update
    mtca4u::TransferElement::readAny(transferElementList);
    
    // trigger read on the poll-type accessors
    for(auto accessor : accessorList) {
      if(accessor.getMode() == UpdateMode::poll) {
        accessor.getAppAccessorNoType().read();
      }
    }
  }

/*********************************************************************************************************************/
  
  VariableNetworkNode VariableGroup::operator()(const std::string& variableName) {
    for(auto variable : getAccessorList()) {
      if(variable.getName() == variableName) return VariableNetworkNode(variable);
    }
    throw std::logic_error("Variable '"+variableName+"' is not part of the variable group '"+_name+"'.");
  }

/*********************************************************************************************************************/

  Module& VariableGroup::operator[](const std::string& moduleName) {
    for(auto submodule : getSubmoduleList()) {
      if(submodule->getName() == moduleName) return *submodule;
    }
    throw std::logic_error("Sub-module '"+moduleName+"' is not part of the variable group '"+_name+"'.");
  }

} /* namespace ChimeraTK */

