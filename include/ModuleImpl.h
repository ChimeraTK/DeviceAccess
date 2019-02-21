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

/**
 *  Some common implementations of a few functions in Module used by most
 * modules (but the VirtualModule).
 */
class ModuleImpl : public Module {

public:
  // constructor inheritances does not work due to a gcc bug!?
  ModuleImpl(EntityOwner *owner, const std::string &name,
             const std::string &description, bool eliminateHierarchy = false,
             const std::unordered_set<std::string> &tags = {})
      : Module(owner, name, description, eliminateHierarchy, tags) {}

  ModuleImpl() : Module() {}

  /** Move constructor */
  ModuleImpl(ModuleImpl &&other) { operator=(std::move(other)); }

  /** Move assignment operator */
  ModuleImpl &operator=(ModuleImpl &&other) {
    if (other.virtualisedModule_isValid)
      virtualisedModule = other.virtualisedModule;
    virtualisedModule_isValid = other.virtualisedModule_isValid;
    Module::operator=(std::forward<ModuleImpl>(other));
    return *this;
  }

  VariableNetworkNode
  operator()(const std::string &variableName) const override;

  Module &operator[](const std::string &moduleName) const override;

  void connectTo(const Module &target,
                 VariableNetworkNode trigger = {}) const override;

  const Module &virtualise() const override;

protected:
  /// Cached return value of virtualise(). Caching is required since
  /// virtualise() returns a reference.
  mutable VirtualModule virtualisedModule{"INVALID", "", ModuleType::Invalid};
  mutable bool virtualisedModule_isValid{false};
};

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_IMPL_H */
