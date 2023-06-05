// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMBackendRegisterInfo.h"
#include "LNMDoubleBufferPlugin.h"
#include "LNMMathPlugin.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  boost::shared_ptr<AccessorPluginBase> makePlugin(LNMBackendRegisterInfo info, size_t pluginIndex,
      const std::string& name, const std::map<std::string, std::string>& parameters) {
    if(name == "multiply") {
      return boost::make_shared<MultiplierPlugin>(info, pluginIndex, parameters);
    }
    if(name == "math") {
      return boost::make_shared<MathPlugin>(info, pluginIndex, parameters);
    }
    if(name == "monostableTrigger") {
      return boost::make_shared<MonostableTriggerPlugin>(info, pluginIndex, parameters);
    }
    if(name == "forceReadOnly") {
      return boost::make_shared<ForceReadOnlyPlugin>(info, pluginIndex, parameters);
    }
    if(name == "forcePollingRead") {
      return boost::make_shared<ForcePollingReadPlugin>(info, pluginIndex, parameters);
    }
    if(name == "typeHintModifier") {
      return boost::make_shared<TypeHintModifierPlugin>(info, pluginIndex, parameters);
    }
    if(name == "doubleBuffer") {
      return boost::make_shared<DoubleBufferPlugin>(info, pluginIndex, parameters);
    }
    if(name == "bitRange") {
      return boost::make_shared<BitRangeAccessPlugin>(info, pluginIndex, parameters);
    }
    throw ChimeraTK::logic_error("LogicalNameMappingBackend: Unknown plugin type '" + name + "'.");
  }

  AccessorPluginBase::AccessorPluginBase(const LNMBackendRegisterInfo& info) : _info(info) {
    // do not hold shared pointers to other plugins or even to yourself inside a plugin.
    _info.plugins.clear();
  }

  void AccessorPluginBase::updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>& catalogue) {
    // First update the info so we have the latest version from the catalogue.
    // At this point we also get a list of shared_ptrs of all plugins inside the info object.
    _info = catalogue.getBackendRegister(_info.name);
    // Do the actual info modifications as implemented by the plugin.
    doRegisterInfoUpdate();
    // Write the modifications back to the catalogue (still including plugins).
    catalogue.modifyRegister(_info);
    // Remove the list of plugins from the copy inside the plugin, which otherwise would hold a shared_ptr to itself.
    // For abstraction reasons it also must not know about other plugins, so it is safe to remove the whole list.
    _info.plugins.clear();
  }

} // namespace ChimeraTK::LNMBackend
