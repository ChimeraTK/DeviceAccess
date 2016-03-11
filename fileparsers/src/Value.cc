/*
 * Value.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "Value.h"

namespace mtca4u {

  template<>
  Value<std::string>& Value<std::string>::operator=(const Value<std::string> &rightHandSide) {
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
