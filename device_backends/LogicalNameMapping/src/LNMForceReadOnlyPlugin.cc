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
      const LNMBackendRegisterInfo& info, const std::map<std::string, std::string>&)
  : AccessorPlugin(info) {}

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

  /** Helper class to implement ForceReadOnlyPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct ForceReadOnlyPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>&) {
      assert(false); // only specialisation is valid
      return {};
    }
  };
  template<typename UserType>
  struct ForceReadOnlyPlugin_Helper<UserType, UserType> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<UserType>>& target) {
      return boost::make_shared<ForceReadOnlyPluginDecorator<UserType>>(target);
    }
  };

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> ForceReadOnlyPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    return ForceReadOnlyPlugin_Helper<UserType, TargetType>::decorateAccessor(target);
  }
} // namespace ChimeraTK::LNMBackend
