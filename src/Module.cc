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
                 bool eliminateHierarchy)
  : EntityOwner(owner, name, description, eliminateHierarchy)
  {}
  
/*********************************************************************************************************************/

  Module::~Module()
  {}
  
/*********************************************************************************************************************/

  void Module::connectTo(const Module &target, VariableNetworkNode trigger) const {
    
    // connect all direct variables of this module to their counter-parts in the right-hand-side module
    for(auto variable : getAccessorList()) {
      if(variable.getDirection() == VariableDirection::feeding) {
        variable >> target(variable.getName());
      }
      else {
        // use trigger?
        if(trigger != VariableNetworkNode() && target(variable.getName()).getMode() == UpdateMode::poll
                                            && variable.getMode() == UpdateMode::push ) {
          target(variable.getName()) [ trigger ] >> variable;
        }
        else {
          target(variable.getName()) >> variable;
        }
      }
    }
    
    // connect all sub-modules to their couter-parts in the right-hand-side module
    for(auto submodule : getSubmoduleList()) {
      submodule->connectTo(target[submodule->getName()], trigger);
    }
    
  }

} /* namespace ChimeraTK */
