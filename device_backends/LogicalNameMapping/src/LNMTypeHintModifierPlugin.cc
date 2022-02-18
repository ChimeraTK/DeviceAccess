#if 0

#  include <boost/make_shared.hpp>

#  include "LNMBackendRegisterInfo.h"
#  include "LNMAccessorPlugin.h"
#  include "NDRegisterAccessor.h"

namespace ChimeraTK { namespace LNMBackend {
  TypeHintModifierPlugin::TypeHintModifierPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info,
      const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<TypeHintModifierPlugin>(info) {
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

  void TypeHintModifierPlugin::updateRegisterInfo() {
    auto info = _info.lock();
    auto d = info->_dataDescriptor;
    auto newDescriptor = RegisterInfoImpl::DataDescriptor(_dataType);
    info->_dataDescriptor = RegisterInfoImpl::DataDescriptor(newDescriptor.fundamentalType(), newDescriptor.isIntegral(),
        newDescriptor.isSigned(), newDescriptor.nDigits(),
        _dataType.isIntegral() ? 0 : newDescriptor.nFractionalDigits(), d.rawDataType(), d.transportLayerDataType());
  }
}} // namespace ChimeraTK::LNMBackend

#endif
