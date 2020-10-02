#include "LNMDoubleBufferPlugin.h"

std::string getValue(std::map<std::string, std::string>& m, std::string key) {
  auto it = m.find(key);
  if(it != m.end()) {
    return it->second;
  }
  else {
    std::string message =
        "LogicalNameMappingBackend DoubleBufferPlugin: Missing parameter " + std::string("'") + key + "'.";
    throw ChimeraTK::logic_error(message);
  }
}

namespace ChimeraTK { namespace LNMBackend {
  DoubleBufferPlugin::DoubleBufferPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, std::map<std::string, std::string> parameters)
  : AccessorPlugin(info), _parameters(std::move(parameters)) {}

  void DoubleBufferPlugin::updateRegisterInfo() {
    auto p = _info.lock();
    if(p) {
      p->writeable = false;
    }
  }
}} // namespace ChimeraTK::LNMBackend
