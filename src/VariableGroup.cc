/*
 * VariableGroup.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationCore.h"

namespace ChimeraTK {

VariableGroup::VariableGroup(EntityOwner *owner, const std::string &name,
                             const std::string &description,
                             bool eliminateHierarchy,
                             const std::unordered_set<std::string> &tags)
    : ModuleImpl(owner, name, description, eliminateHierarchy, tags) {
  if (!dynamic_cast<ApplicationModule *>(owner) &&
      !dynamic_cast<DeviceModule *>(owner) &&
      !dynamic_cast<VariableGroup *>(owner)) {
    std::cout << "VariableGroup is throwing" << std::endl;
    throw ChimeraTK::logic_error(
        "VariableGroups must be owned by ApplicationModule, DeviceModule or "
        "other VariableGroups!");
  }
}

} /* namespace ChimeraTK */
