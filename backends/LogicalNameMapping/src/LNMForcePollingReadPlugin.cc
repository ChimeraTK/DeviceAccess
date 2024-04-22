// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessor.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  ForcePollingReadPlugin::ForcePollingReadPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>&)
  : AccessorPlugin(info, pluginIndex) {}

  /********************************************************************************************************************/

  void ForcePollingReadPlugin::doRegisterInfoUpdate() {
    if(_info.supportedFlags.has(AccessMode::wait_for_new_data)) {
      _info.supportedFlags.remove(AccessMode::wait_for_new_data);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> ForcePollingReadPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if(target->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "AccessMode::wait_for_new_data is disallowed through ForcePollingReadPlugin for register '" +
          target->getName() + "'.");
    }

    if constexpr(std::is_same<UserType, TargetType>::value) {
      return target;
    }

    assert(false);

    return {};
  }
} // namespace ChimeraTK::LNMBackend
