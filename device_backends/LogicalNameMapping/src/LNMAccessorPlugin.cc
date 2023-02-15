// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMBackendRegisterInfo.h"
#include "LNMDoubleBufferPlugin.h"
#include "NDRegisterAccessorDecorator.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  boost::shared_ptr<AccessorPluginBase> makePlugin(
      LNMBackendRegisterInfo info, const std::string& name, const std::map<std::string, std::string>& parameters) {
    if(name == "multiply") {
      return boost::make_shared<MultiplierPlugin>(info, parameters);
    }
    if(name == "math") {
      return boost::make_shared<MathPlugin>(info, parameters);
    }
    if(name == "monostableTrigger") {
      return boost::make_shared<MonostableTriggerPlugin>(info, parameters);
    }
    if(name == "forceReadOnly") {
      return boost::make_shared<ForceReadOnlyPlugin>(info, parameters);
    }
    if(name == "forcePollingRead") {
      return boost::make_shared<ForcePollingReadPlugin>(info, parameters);
    }
    if(name == "typeHintModifier") {
      return boost::make_shared<TypeHintModifierPlugin>(info, parameters);
    }
    if(name == "doubleBuffer") {
      return boost::make_shared<DoubleBufferPlugin>(info, parameters);
    }
    throw ChimeraTK::logic_error("LogicalNameMappingBackend: Unknown plugin type '" + name + "'.");
  }

} // namespace ChimeraTK::LNMBackend
