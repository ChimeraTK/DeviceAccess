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
    bool gotUpdate = false;
    auto accessorList = getAccessorListRecursive();
    while(!gotUpdate) {     /// @todo TODO FIXME make proper blocking implementation
      boost::this_thread::yield();
      boost::this_thread::interruption_point();
      
      for(auto accessor : accessorList) {       // @todo FIXME make sure no submodule is accessing the variables itself... (e.g. by forcing all submodules to be a VariablGroup and not e.g. an ApplicationModule)
        if(accessor->getUpdateMode() == UpdateMode::push) {
          if(accessor->readNonBlocking()) gotUpdate = true;
        }
      }
    }
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

