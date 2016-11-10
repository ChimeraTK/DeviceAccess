/*
 * ControlSystemModule.h
 *
 *  Created on: Jun 28, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONTROL_SYSTEM_MODULE_H
#define CHIMERATK_CONTROL_SYSTEM_MODULE_H

#include <mtca4u/RegisterPath.h>

#include "VariableNetworkNode.h"
#include "Module.h"

namespace ChimeraTK {

  class ControlSystemModule : public Module {

    public:

      /** Constructor: the optional variableNamePrefix will be prepended to all control system variable names
       *  (separated by a slash). */
      ControlSystemModule(const std::string& variableNamePrefix="");

      /** The subscript operator returns a VariableNetworkNode which can be used in the Application::initialise()
       *  function to connect the control system variable with another variable. */
      VariableNetworkNode operator()(const std::string& variableName, const std::type_info &valueType=typeid(AnyType),
                                     size_t nElements=0);

    protected:

      mtca4u::RegisterPath variableNamePrefix;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONTROL_SYSTEM_MODULE_H */
