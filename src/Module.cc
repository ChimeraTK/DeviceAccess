/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "Module.h"
#include "Accessor.h"

namespace ChimeraTK {

  Module::Module(EntityOwner *owner, const std::string &name)
  : EntityOwner(owner, name)
  {
    Application *theApp = &(Application::getInstance());
    assert(theApp != nullptr);
    theApp->overallRegisterModule(*this);
  }
  
/*********************************************************************************************************************/

  Module& Module::operator>=(Module &rhs) {
    
    // connect all direct variables of this module to their counter-parts in the right-hand-side module
    for(auto variable : getAccessorList()) {
      if(variable->getDirection() == VariableDirection::feeding) {
        variable->getNode() >> rhs(variable->getName());
      }
      else {
        rhs(variable->getName()) >> variable->getNode();
      }
    }
    
    // connect all sub-modules to their couter-parts in the right-hand-side module
    for(auto submodule : getSubmoduleList()) {
      *submodule >= rhs[submodule->getName()];
    }
    
    return *this;
    
  }

} /* namespace ChimeraTK */
