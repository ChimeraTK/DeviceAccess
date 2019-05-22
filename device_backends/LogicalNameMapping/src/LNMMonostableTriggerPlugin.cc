#include <boost/make_shared.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK { namespace LNMBackend {

  /********************************************************************************************************************/

  MonostableTriggerPlugin::MonostableTriggerPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info) {
    // extract parameters
    if(parameters.find("milliseconds") == parameters.end()) {
      throw ChimeraTK::logic_error(
          "LogicalNameMappingBackend MonostableTriggerPluginPlugin: Missing parameter 'milliseconds'.");
    }
    _milliseconds = std::stod(parameters.at("milliseconds"));
    if(parameters.find("active") != parameters.end()) {
      _active = std::stoul(parameters.at("active"));
    }
    if(parameters.find("inactive") != parameters.end()) {
      _inactive = std::stoul(parameters.at("inactive"));
    }

    // Change register info to write-only and data type nodata
    info->readable = false;
    info->_dataDescriptor = ChimeraTK::RegisterInfo::DataDescriptor(RegisterInfo::FundamentalType::nodata);
  }

  /********************************************************************************************************************/

  void MonostableTriggerPlugin::updateRegisterInfo() {
    // Change register info to write-only and data type nodata
    _info->readable = false;
    _info->_dataDescriptor = ChimeraTK::RegisterInfo::DataDescriptor(RegisterInfo::FundamentalType::nodata);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct MonostableTriggerPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, uint32_t> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, uint32_t>::buffer_2D;

    MonostableTriggerPluginDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>>& target,
        double milliseconds, uint32_t active, uint32_t inactive)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType, uint32_t>(target), _delay(milliseconds), _active(active),
      _inactive(inactive) {
      // make sure the target register is writeable and scalar
      if(!_target->isWriteable()) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend MonostableTriggerPluginPlugin: Cannot target non-writeable register.");
      }
      if(_target->getNumberOfChannels() > 1 || _target->getNumberOfSamples() > 1) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend MonostableTriggerPluginPlugin: Cannot target non-scalar registers.");
      }
    }

    bool isReadable() const override { return false; }

    void doPreRead() override {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend MonostableTriggerPluginPlugin: Reading is not allowed.");
    }

    void doPostRead() override {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend MonostableTriggerPluginPlugin: Reading is not allowed.");
    }

    void doPreWrite() override {}

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override;

    void doPostWrite() override {}

    void interrupt() override { _target->interrupt(); }

    ChimeraTK::VersionNumber getVersionNumber() const override { return _target->getVersionNumber(); }

    std::chrono::duration<double, std::milli> _delay;
    uint32_t _active, _inactive;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, uint32_t>::_target;
  };

  template<typename UserType>
  bool MonostableTriggerPluginDecorator<UserType>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    _target->accessData(0, 0) = _active;
    bool a = _target->write(versionNumber);
    std::this_thread::sleep_for(_delay);
    _target->accessData(0, 0) = _inactive;
    bool b = _target->write(versionNumber);
    return a || b;
  }

  /********************************************************************************************************************/

  /** Helper class to implement MonostableTriggerPluginPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct MonostableTriggerPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>&, double, uint32_t, uint32_t) {
      assert(false); // only specialisation is valid
      return {};
    }
  };
  template<typename UserType>
  struct MonostableTriggerPlugin_Helper<UserType, uint32_t> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<uint32_t>>& target, double milliseconds, uint32_t active,
        uint32_t inactive) {
      return boost::make_shared<MonostableTriggerPluginDecorator<UserType>>(target, milliseconds, active, inactive);
    }
  };

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MonostableTriggerPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    return MonostableTriggerPlugin_Helper<UserType, TargetType>::decorateAccessor(
        target, _milliseconds, _active, _inactive);
  }
}} // namespace ChimeraTK::LNMBackend
