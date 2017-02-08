/*
 * VariableGroup.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "VariableGroup.h"
#include "Accessor.h"

namespace ChimeraTK {

  VariableGroup::~VariableGroup() {
  }

/*********************************************************************************************************************/
  
  void VariableGroup::readAny() {
    auto accessorList = getAccessorListRecursive();

    // put push-type transfer elements into a list suitable for TransferElement::readAny()
    std::list<std::reference_wrapper<TransferElement>> transferElementList;
    for(auto &accessor : accessorList) {
      if(accessor->getUpdateMode() == UpdateMode::push) {
        transferElementList.emplace_back(*(accessor->getTransferElement()));
      }
    }
    
    // wait until one of the push-type accessors receives an update
    mtca4u::TransferElement::readAny(transferElementList);
    
    // trigger read on the poll-type accessors
    for(auto accessor : accessorList) {
      if(accessor->getUpdateMode() == UpdateMode::poll) {
        accessor->read();
      }
    }
  }

/*********************************************************************************************************************/
  
  VariableNetworkNode VariableGroup::operator()(const std::string& variableName) {
    for(auto variable : getAccessorList()) {
      if(variable->getName() == variableName) return VariableNetworkNode(*variable);
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

