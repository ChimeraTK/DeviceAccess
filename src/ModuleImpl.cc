#include "ModuleImpl.h"

namespace ChimeraTK {
  
/*********************************************************************************************************************/
  
  VariableNetworkNode ModuleImpl::operator()(const std::string& variableName) const {
    return virtualise()(variableName);
  }

/*********************************************************************************************************************/

  Module& ModuleImpl::operator[](const std::string& moduleName) const {
    return virtualise()[moduleName];
  }

/*********************************************************************************************************************/

  void ModuleImpl::connectTo(const Module &target, VariableNetworkNode trigger) const {
    virtualise().connectTo(target.virtualise(), trigger);
  }

/*********************************************************************************************************************/

  const Module& ModuleImpl::virtualise() const {
    if(!virtualisedModule_isValid) {
      virtualisedModule = findTag(".*");
      virtualisedModule_isValid = true;
    }
    return virtualisedModule;
  }

} // namespace ChimeraTK
