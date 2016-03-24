/*
 * DynamicValue.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "DynamicValue.h"

namespace mtca4u {

  template<>
  DynamicValue<std::string>& DynamicValue<std::string>::operator=(const DynamicValue<std::string> &rightHandSide) {
    if(rightHandSide.hasActualValue) {
      hasActualValue = true;
      value = rightHandSide.value;
    }
    else {
      // we obtain the register accessor later, in case the map file was not yet parsed up to its definition
      hasActualValue = false;
      registerName = rightHandSide.registerName;
      accessor.reset();
    }
    return *this;
  }

} /* namespace mtca4u */
