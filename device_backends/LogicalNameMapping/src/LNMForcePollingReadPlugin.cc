#include <boost/make_shared.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"

namespace ChimeraTK { namespace LNMBackend {

  /********************************************************************************************************************/

  ForcePollingReadPlugin::ForcePollingReadPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>&)
  : AccessorPlugin(info) {}

  /********************************************************************************************************************/

  void ForcePollingReadPlugin::updateRegisterInfo() {
    // remove wait_for_new_data flag, if present
    if(_info->supportedFlags.has(AccessMode::wait_for_new_data)) {
      _info->supportedFlags.remove(AccessMode::wait_for_new_data);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> ForceReadOnlyPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    if(target->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "AccessMode::wait_for_new_data is disallowed through ForcePollingReadPlugin for register '" +
          target->getName() + "'.");
    }
    return target;
  }
}} // namespace ChimeraTK::LNMBackend
