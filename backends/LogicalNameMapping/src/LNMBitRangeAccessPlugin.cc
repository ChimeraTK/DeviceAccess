// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BitRangeAccessorDecorator.h"
#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMappingBackend.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <boost/make_shared.hpp>

#include <charconv>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  BitRangeAccessPlugin::BitRangeAccessPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<BitRangeAccessPlugin>(info, pluginIndex) {
    try {
      const auto& shift = parameters.at("shift");

      // This is how you are supposed to use std::from_chars with std::string
      // NOLINTNEXTLINE(unsafe-buffer-usage)
      auto [suffix, ec]{std::from_chars(shift.data(), shift.data() + shift.size(), _shift)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "shift".)");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
          R"(: Missing parameter "shift".)");
    }

    try {
      const auto& numberOfBits = parameters.at("numberOfBits");
      // This is how you are supposed to use std::from_chars with std::string
      // NOLINTNEXTLINE(unsafe-buffer-usage)
      auto [suffix, ec]{std::from_chars(numberOfBits.data(), numberOfBits.data() + numberOfBits.size(), _numberOfBits)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "numberOfBits".)");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
          R"(: Missing parameter "numberOfBits".)");
    }

    if(const auto it = parameters.find("fractionalBits"); it != parameters.end()) {
      // This is how you are supposed to use std::from_chars with std::string
      // NOLINTNEXTLINE(unsafe-buffer-usage)
      auto [suffix, ec]{
          std::from_chars(it->second.data(), it->second.data() + it->second.size(), dataInterpretationFractionalBits)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "fractionalBits".)");
      }
    }

    if(const auto it = parameters.find("signed"); it != parameters.end()) {
      std::stringstream ss(it->second);
      Boolean value;
      ss >> value;
      dataInterpretationIsSigned = value;
    }
  }

  /********************************************************************************************************************/

  void BitRangeAccessPlugin::doRegisterInfoUpdate() {
    // We do not support wait_for_new_data with this decorator
    _info.supportedFlags.remove(AccessMode::wait_for_new_data);
    _info.supportedFlags.remove(AccessMode::raw);
    // also remove raw-type info from DataDescriptor
    _info._dataDescriptor.setRawDataType(DataType::none);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> BitRangeAccessPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams& params) {
    if constexpr(std::is_same_v<TargetType, uint64_t>) {
      if(params._flags.has(AccessMode::wait_for_new_data) || params._flags.has(AccessMode::raw)) {
        throw ChimeraTK::logic_error(
            std::format("BitRangePlugin (on {}) Unsupported flags in {}", params._name, params._flags.serialize()));
      }

      // Resolve the correct target device
      std::string devName = _info.deviceName;
      boost::shared_ptr<DeviceBackend> targetDevice;
      if(devName != "this") {
        targetDevice = backend->_devices[devName];
      }
      else {
        targetDevice = backend;
      }

      RegisterPath targetPath{params._name};

      if(params._numberOfWords > 1 || params._wordOffsetInRegister != 0) {
        throw ChimeraTK::logic_error("LNMBitRangeAccessPlugin: target must be scalar, register name: " + _info.name);
      }

      NumericAddressedRegisterInfo regInfo(_info.name, 1, 0, sizeof(uint64_t), 0, _numberOfBits,
          dataInterpretationFractionalBits, dataInterpretationIsSigned,
          _info.isWriteable() ? NumericAddressedRegisterInfo::Access::READ_WRITE :
                                NumericAddressedRegisterInfo::Access::READ_ONLY);
      regInfo.channels[0].bitOffset = _shift;

      return boost::make_shared<detail::BitRangeAccessorDecorator<UserType, false>>(
          targetDevice, targetPath, target, regInfo, 1, 0);
    }

    assert(false);

    return {};
  }

} // namespace ChimeraTK::LNMBackend
