#include "ApplicationCore.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  VariableNetworkNode ModuleImpl::operator()(const std::string& variableName) const {
    return virtualise()(variableName);
  }

  /*********************************************************************************************************************/

  Module& ModuleImpl::operator[](const std::string& moduleName) const { return virtualise()[moduleName]; }

  /*********************************************************************************************************************/

  void ModuleImpl::connectTo(const Module& target, VariableNetworkNode trigger) const {
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

  /*********************************************************************************************************************/

  ConfigReader& ModuleImpl::appConfig() const {
    size_t nConfigReaders = 0;
    ConfigReader* instance = nullptr;
    for(auto* mod : Application::getInstance().getSubmoduleListRecursive()) {
      if(!dynamic_cast<ConfigReader*>(mod)) continue;
      ++nConfigReaders;
      instance = dynamic_cast<ConfigReader*>(mod);
    }
    if(nConfigReaders != 1) {
      std::string message = "ApplicationModule::appConfig() called but " + std::to_string(nConfigReaders) +
                            " instances of ChimeraTK::ConfigReader have been found.";
      // Printing the message as well; there is a situation when running under Boost::Test where this
      // is caught by Boost and causes a weird destructor message from AppBase.cc instead with no means of
      // finding out the actual error
      std::cerr << message << std::endl;
      throw ChimeraTK::logic_error(message);
    }
    return *instance;
  }
} // namespace ChimeraTK
