/*
 * ModuleGroup.cc
 *
 *  Created on: Aug 23, 2017
 *      Author: Martin Hierholzer
 */

#include "ApplicationCore.h"

namespace ChimeraTK {

  ModuleGroup::ModuleGroup(EntityOwner *owner, const std::string &name, const std::string &description,
          bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
  : ModuleImpl(owner,name,description,eliminateHierarchy,tags)
  {
    if(!dynamic_cast<Application*>(owner) && !dynamic_cast<ModuleGroup*>(owner)) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
        "ModuleGroups must be owned either by the Application or other ModuleGroups!");
    }
  }

} /* namespace ChimeraTK */


