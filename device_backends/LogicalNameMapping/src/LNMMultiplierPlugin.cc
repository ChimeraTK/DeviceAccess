// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  MultiplierPlugin::MultiplierPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info, pluginIndex) {
    // extract parameters
    if(parameters.find("factor") == parameters.end()) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend MultiplierPlugin: Missing parameter 'factor'.");
    }
    _factor = std::stod(parameters.at("factor"));
  }

  /********************************************************************************************************************/

  void MultiplierPlugin::doRegisterInfoUpdate() {
    _info._dataDescriptor = ChimeraTK::DataDescriptor(DataType("float64"));
    _info.supportedFlags.remove(AccessMode::raw);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct MultiplierPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;

    MultiplierPluginDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target, double factor)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target), _factor(factor) {}

    void doPreRead(TransferType type) override { _target->preRead(type); }

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreWrite(TransferType type, VersionNumber) override;

    void doPostWrite(TransferType type, VersionNumber dataLost) override { _target->postWrite(type, dataLost); }

    double _factor;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;
  };

  template<typename UserType>
  void MultiplierPluginDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    _target->postRead(type, hasNewData);
    if(!hasNewData) return;
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      for(size_t k = 0; k < _target->getNumberOfSamples(); ++k) {
        buffer_2D[i][k] = numericToUserType<UserType>(_target->accessData(i, k) * _factor);
      }
    }
    this->_versionNumber = _target->getVersionNumber();
    this->_dataValidity = _target->dataValidity();
  }

  template<typename UserType>
  void MultiplierPluginDecorator<UserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      for(size_t k = 0; k < _target->getNumberOfSamples(); ++k) {
        _target->accessData(i, k) = userTypeToNumeric<double>(buffer_2D[i][k]) * _factor;
      }
    }
    _target->setDataValidity(this->_dataValidity);
    _target->preWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MultiplierPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if constexpr(std::is_same<TargetType, double>::value) {
      return boost::make_shared<MultiplierPluginDecorator<UserType>>(target, _factor);
    }

    assert(false);

    return {};
  }
} // namespace ChimeraTK::LNMBackend
