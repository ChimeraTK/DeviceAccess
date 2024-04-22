// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  ForceReadOnlyPlugin::ForceReadOnlyPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>&)
  : AccessorPlugin(info, pluginIndex) {}

  /********************************************************************************************************************/

  void ForceReadOnlyPlugin::doRegisterInfoUpdate() {
    // Change register info to read-only
    _info.writeable = false;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct ForceReadOnlyPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D;

    explicit ForceReadOnlyPluginDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType>(target) {
      // make sure the target register is writeable and scalar
      if(!target->isReadable()) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend ForceReadOnlyPlugin: Cannot target non-readable register.");
      }
    }

    [[nodiscard]] bool isWriteable() const override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend ForceReadOnlyPlugin: Writing is not allowed.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> ForceReadOnlyPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if constexpr(std::is_same<UserType, TargetType>::value) {
      return boost::make_shared<ForceReadOnlyPluginDecorator<UserType>>(target);
    }

    assert(false);

    return {};
  }
} // namespace ChimeraTK::LNMBackend
