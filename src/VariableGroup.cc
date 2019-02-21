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
      !dynamic_cast<VariableGroup *>(owner)) {
    throw ChimeraTK::logic_error("VariableGroups must be owned either by "
                                 "ApplicationModule or other VariableGroups!");
  }
}

} /* namespace ChimeraTK */
