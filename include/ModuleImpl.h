/*
 * ModuleImpl.h
 *
 *  Created on: Okt 11, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_IMPL_H
#define CHIMERATK_MODULE_IMPL_H

#include "Module.h"
#include "VirtualModule.h"

namespace ChimeraTK {

  struct ConfigReader;

  /**
   *  Some common implementations of a few functions in Module used by most
   * modules (but the VirtualModule).
   */
  class ModuleImpl : public Module {
   public:
    // constructor inheritances does not work due to a gcc bug!?
    ModuleImpl(EntityOwner* owner, const std::string& name, const std::string& description,
        HierarchyModifier hierarchyModifier = HierarchyModifier::none, const std::unordered_set<std::string>& tags = {})
    : Module(owner, name, description, hierarchyModifier, tags) {}

    ModuleImpl(EntityOwner* owner, const std::string& name, const std::string& description, bool eliminateHierarchy,
        const std::unordered_set<std::string>& tags = {})
    : Module(owner, name, description, eliminateHierarchy, tags) {}

    ModuleImpl() : Module() {}

    /** Move constructor */
    ModuleImpl(ModuleImpl&& other) { operator=(std::move(other)); }

    /** Move assignment operator */
    ModuleImpl& operator=(ModuleImpl&& other) {
      if(other.virtualisedModule_isValid) virtualisedModule = other.virtualisedModule;
      virtualisedModule_isValid = other.virtualisedModule_isValid;
      Module::operator=(std::forward<ModuleImpl>(other));
      return *this;
    }

    VariableNetworkNode operator()(const std::string& variableName) const override;

    Module& operator[](const std::string& moduleName) const override;

    void connectTo(const Module& target, VariableNetworkNode trigger = {}) const override;

    const Module& virtualise() const override;

    /** Obtain the ConfigReader instance of the application. If no or multiple ConfigReader instances are found, a
     *  ChimeraTK::logic_error is thrown.
     *  Note: This function is expensive. It should be called only during the constructor of the ApplicationModule and
     *  the obtained configuration values should be stored for later use in member variables.
     *  Beware that the ConfigReader instance can obly be found if it has been constructed before calling this function.
     *  Hence, the Application should have the ConfigReader as its first member. */
    ConfigReader& appConfig() const;

   protected:
    /// Cached return value of virtualise(). Caching is required since
    /// virtualise() returns a reference.
    mutable VirtualModule virtualisedModule{"INVALID", "", ModuleType::Invalid};
    mutable bool virtualisedModule_isValid{false};
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_IMPL_H */
