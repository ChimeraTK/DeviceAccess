/*
 * ModuleGroup.h
 *
 *  Created on: Aug 23, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_GROUP_H
#define CHIMERATK_MODULE_GROUP_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"

namespace ChimeraTK {

  class ModuleGroup : public Module {

    public:
      
      /** Constructor: register the ModuleGroup with its owner. If eliminateHierarchy is true, the hierarchy level
       *  introduced by this group will be eliminated from the "dynamic" data model (see
       *  EntityOwner::setEliminateHierarchy()). The tags given as the last argument are added to all variables
       *  in this module recursively (see EntityOwner::addTag()).
       *
       *  Note: ModuleGroups may only be owned by the Application or other ModuleGroups. */
      ModuleGroup(EntityOwner *owner, const std::string &name, const std::string &description,
             bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

      /** Default constructor: Allows late initialisation of VariableGroups (e.g. when creating arrays of
       *  VariableGroups).
       * 
       *  This construtor also has to be here to mitigate a bug in gcc. It is needed to allow constructor
       *  inheritance of modules owning other modules. This constructor will not actually be called then.
       *  See this bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
      ModuleGroup() : Module(nullptr, "invalid", "invalid VariableGroup") {}

      /** Move operation with the move constructor */
      ModuleGroup(ModuleGroup &&other) :
      Module(std::move(other))
      {}
      
      /** Inherit assignment */
      using Module::operator=;

      /** Destructor */
      virtual ~ModuleGroup() {};

      ModuleType getModuleType() const override { return ModuleType::ModuleGroup; }

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_GROUP_H */

