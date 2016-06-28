/*
 * DeviceModule.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include <mtca4u/DeviceBackend.h>

#include "Application.h"
#include "DeviceModule.h"

namespace ChimeraTK {

  DeviceModule::DeviceModule(const std::string& _deviceAliasOrURI, const std::string& _registerNamePrefix)
  : deviceAliasOrURI(_deviceAliasOrURI), registerNamePrefix(_registerNamePrefix)
  {
  }

  /*********************************************************************************************************************/

  VariableNetworkNode DeviceModule::operator()(const std::string& registerName, UpdateMode mode,
      const std::type_info &valueType) {
    return{deviceAliasOrURI, registerNamePrefix/registerName, mode, VariableDirection::invalid, valueType};
  }

  /*********************************************************************************************************************/

  void DeviceModule::prepare() {

  }

}

