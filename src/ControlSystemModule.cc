/*
 * ControlSystemModule.cc
 *
 *  Created on: Jun 28, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "ControlSystemModule.h"

namespace ChimeraTK {

  ControlSystemModule::ControlSystemModule(const std::string& _variableNamePrefix)
  : variableNamePrefix(_variableNamePrefix)
  {
  }

  /*********************************************************************************************************************/

  VariableNetworkNode ControlSystemModule::operator()(const std::string& variableName, const std::type_info &valueType) {
    return{variableNamePrefix/variableName, VariableDirection::invalid, valueType};
  }

}
