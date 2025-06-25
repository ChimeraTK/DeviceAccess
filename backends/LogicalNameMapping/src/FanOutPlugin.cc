// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  FanOutPlugin::FanOutPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info, pluginIndex) {
    // extract parameters
    for(const auto& [param, value] : parameters) {
      if(boost::starts_with(param, "target")) {
        _targets.push_back(value);
      }
      else {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend FanOutPlugin: Unknown parameter '" + param + "'.");
      }
    }
  }

  /********************************************************************************************************************/

  void FanOutPlugin::doRegisterInfoUpdate() {
    if(!_info.writeable) {
      throw ChimeraTK::logic_error("FanOutPlugin requires a writeable target register: " + _info.getRegisterName());
    }

    _info.readable = false;
    _info.supportedFlags.remove(AccessMode::raw);
    _info._dataDescriptor.setRawDataType(DataType::Void);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  struct FanOutPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, UserType> {
    FanOutPluginDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target,
        std::vector<boost::shared_ptr<NDRegisterAccessor<UserType>>> accs)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType, UserType>(target), _accs(std::move(accs)) {}

    void doPreWrite(TransferType type, VersionNumber) override;
    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;
    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override;
    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    [[nodiscard]] bool isReadOnly() const override { return false; }
    [[nodiscard]] bool isReadable() const override { return false; }

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, UserType>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, UserType>::_target;

   private:
    std::vector<boost::shared_ptr<NDRegisterAccessor<UserType>>> _accs;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  void FanOutPluginDecorator<UserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    std::exception_ptr firstException{nullptr};

    for(auto& acc : _accs) {
      for(size_t i = 0; i < acc->getNumberOfChannels(); ++i) {
        acc->accessChannel(i) = buffer_2D[i];
      }
      acc->setDataValidity(this->_dataValidity);
      try {
        acc->preWrite(type, versionNumber);
      }
      catch(...) {
        if(!firstException) {
          firstException = std::current_exception();
        }
      }
    }

    // the base class' doPreWrite must be called even if the other preWrites throw, otherwise our postWrite destroys the
    // application buffer with its swap.
    try {
      NDRegisterAccessorDecorator<UserType, UserType>::doPreWrite(type, versionNumber);
    }
    catch(...) {
      if(!firstException) {
        firstException = std::current_exception();
      }
    }

    // if any exception was thrown, re-throw the first one
    if(firstException) {
      std::rethrow_exception(firstException);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool FanOutPluginDecorator<UserType>::doWriteTransfer(VersionNumber versionNumber) {
    bool rv = false;

    for(auto& acc : _accs) {
      rv |= acc->writeTransfer(versionNumber);
    }

    rv |= NDRegisterAccessorDecorator<UserType, UserType>::doWriteTransfer(versionNumber);

    return rv;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool FanOutPluginDecorator<UserType>::doWriteTransferDestructively(VersionNumber versionNumber) {
    bool rv = false;

    for(auto& acc : _accs) {
      rv |= acc->writeTransferDestructively(versionNumber);
    }

    rv |= NDRegisterAccessorDecorator<UserType, UserType>::doWriteTransferDestructively(versionNumber);

    return rv;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void FanOutPluginDecorator<UserType>::doPostWrite(TransferType type, VersionNumber versionNumber) {
    for(auto& acc : _accs) {
      acc->postWrite(type, versionNumber);
    }

    NDRegisterAccessorDecorator<UserType, UserType>::doPostWrite(type, versionNumber);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> FanOutPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if(!target->isWriteable()) {
      throw ChimeraTK::logic_error(std::format("LogicalNameMappingBackend FanOutPlugin: Main target register "
                                               "'{}' is not writeable.",
          target->getName()));
    }

    if(target->getAccessModeFlags().has(AccessMode::raw)) {
      throw ChimeraTK::logic_error(
          std::format("LogicalNameMappingBackend FanOutPlugin: AccessMode::raw is not supported in register '{}'.",
              target->getName()));
    }

    // create additional target accessors
    std::vector<boost::shared_ptr<NDRegisterAccessor<TargetType>>> accs;
    accs.reserve(_targets.size());
    for(const auto& name : _targets) {
      accs.push_back(backend->getRegisterAccessor<TargetType>(name, 0, 0, {}));
      if(accs.back()->getNumberOfChannels() != target->getNumberOfChannels() ||
          accs.back()->getNumberOfSamples() != target->getNumberOfSamples()) {
        throw ChimeraTK::logic_error(std::format("LogicalNameMappingBackend FanOutPlugin: Shape of target register "
                                                 "'{}' does not match the shape of the main target {}.",
            name, target->getName()));
      }
      if(!accs.back()->isWriteable()) {
        throw ChimeraTK::logic_error(std::format("LogicalNameMappingBackend FanOutPlugin: Target register "
                                                 "'{}' is not writeable (main target: {}).",
            name, target->getName()));
      }
    }

    if constexpr(std::is_same<TargetType, UserType>::value) {
      return boost::make_shared<FanOutPluginDecorator<UserType>>(target, std::move(accs));
    }

    assert(false);

    return {};
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::LNMBackend
