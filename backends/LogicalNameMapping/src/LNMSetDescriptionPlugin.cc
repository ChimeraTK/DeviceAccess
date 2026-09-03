// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessorDecorator.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  SetDescriptionPlugin::SetDescriptionPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info, pluginIndex) {
    auto unitIt = parameters.find("engineeringUnit");
    if(unitIt != parameters.end()) {
      _engineeringUnit = unitIt->second;
    }
    auto descriptionIt = parameters.find("description");
    if(descriptionIt != parameters.end()) {
      _description = descriptionIt->second;
    }
  }

  /********************************************************************************************************************/

  void SetDescriptionPlugin::doRegisterInfoUpdate() {
    // Update the catalogue info so tooling can see the overwritten values before creating an accessor.
    // Only overwrite the fields that were explicitly provided via the plugin parameters.
    if(_engineeringUnit) {
      _info.engineeringUnit = *_engineeringUnit;
    }
    if(_description) {
      _info.description = *_description;
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct SetDescriptionPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType> {
    explicit SetDescriptionPluginDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target,
        const std::optional<std::string>& engineeringUnit, const std::optional<std::string>& description)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType>(target) {
      if(engineeringUnit) {
        this->_unit = *engineeringUnit;
      }
      if(description) {
        this->_description = *description;
      }
    }
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> SetDescriptionPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if constexpr(std::is_same<UserType, TargetType>::value) {
      return boost::make_shared<SetDescriptionPluginDecorator<UserType>>(target, _engineeringUnit, _description);
    }

    assert(false); // SetDescriptionPlugin does not change the user type

    return {};
  }

} // namespace ChimeraTK::LNMBackend
