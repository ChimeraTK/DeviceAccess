#include <boost/make_shared.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessor.h"
#include "DataDescriptor.h"

namespace ChimeraTK { namespace LNMBackend {
  TypeHintModifierPlugin::TypeHintModifierPlugin(
      LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters)
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

  void TypeHintModifierPlugin::updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>& catalogue) {
    // first update the info so we have the latest version from the catalogue.
    _info = catalogue.getBackendRegister(_info.name);
    auto d = _info._dataDescriptor;
    auto newDescriptor = DataDescriptor(_dataType);
    _info._dataDescriptor = DataDescriptor(newDescriptor.fundamentalType(), newDescriptor.isIntegral(),
        newDescriptor.isSigned(), newDescriptor.nDigits(),
        _dataType.isIntegral() ? 0 : newDescriptor.nFractionalDigits(), d.rawDataType(), d.transportLayerDataType());
    catalogue.modifyRegister(_info);
  }
}} // namespace ChimeraTK::LNMBackend
