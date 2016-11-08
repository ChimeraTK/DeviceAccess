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
    while(!gotUpdate) {     /// @todo TODO FIXME make proper blocking implementation
      boost::this_thread::yield();
      boost::this_thread::interruption_point();
      for(auto accessor : accessorList) {
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
  
} /* namespace ChimeraTK */

