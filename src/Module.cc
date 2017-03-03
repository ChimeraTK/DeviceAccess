/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "Module.h"

namespace ChimeraTK {

  Module::Module(EntityOwner *owner, const std::string &name)
  : EntityOwner(owner, name)
  {}
  
/*********************************************************************************************************************/

  Module::~Module()
  {}
  
/*********************************************************************************************************************/

  Module& Module::operator>=(Module &rhs) {
    
    // connect all direct variables of this module to their counter-parts in the right-hand-side module
    for(auto variable : getAccessorList()) {
      if(variable.getDirection() == VariableDirection::feeding) {
        variable >> rhs(variable.getName());
      }
      else {
        rhs(variable.getName()) >> variable;
      }
    }
    
    // connect all sub-modules to their couter-parts in the right-hand-side module
    for(auto submodule : getSubmoduleList()) {
      *submodule >= rhs[submodule->getName()];
    }
    
    return *this;
    
  }

} /* namespace ChimeraTK */
