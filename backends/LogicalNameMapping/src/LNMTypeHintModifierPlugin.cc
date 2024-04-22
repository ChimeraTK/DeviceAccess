// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessor.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::LNMBackend {
  TypeHintModifierPlugin::TypeHintModifierPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<TypeHintModifierPlugin>(info, pluginIndex) {
    try {
      std::string typeName = parameters.at("type");
      if(typeName == "integer") typeName = "int32";

      _dataType = DataType(typeName);
      if(_dataType == DataType::none) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend TypeHintModifierPlugin: Unknown type '" + typeName + "'.");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend TypeHintModifierPlugin: Missing parameter 'type'.");
    }
  }

  void TypeHintModifierPlugin::doRegisterInfoUpdate() {
    // keep raw data type and transport layer data type from original entry
    auto d = _info._dataDescriptor;

    auto newDescriptor = DataDescriptor(_dataType);

    if(newDescriptor.fundamentalType() == DataDescriptor::FundamentalType::numeric) {
      _info._dataDescriptor = DataDescriptor(newDescriptor.fundamentalType(), newDescriptor.isIntegral(),
          newDescriptor.isSigned(), newDescriptor.nDigits(),
          _dataType.isIntegral() ? 0 : newDescriptor.nFractionalDigits(), d.rawDataType(), d.transportLayerDataType());
    }
    else {
      // can't call isIntegral() etc. if not numeric (just provide dummy info for those fields as they are ignored)
      _info._dataDescriptor = DataDescriptor(
          newDescriptor.fundamentalType(), false, false, 0, 0, d.rawDataType(), d.transportLayerDataType());
    }

    d = _info._dataDescriptor;
  }
} // namespace ChimeraTK::LNMBackend
