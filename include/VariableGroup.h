/*
 * VariableGroup.h
 *
 *  Created on: Nov 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_GROUP_H
#define CHIMERATK_VARIABLE_GROUP_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"

namespace ChimeraTK {
  
  class ApplicationModule;

  class VariableGroup : public Module {

    public:
      
      /** Constructor: register the VariableGroup with its owner. If eliminateHierarchy is true, the hierarchy level
       *  introduced by this group will be eliminated from the "dynamic" data model (see
       *  EntityOwner::setEliminateHierarchy()). The tags given as the last argument are added to all variables
       *  in this group recursively (see EntityOwner::addTag()).
       *
       *  Note: VariableGroups may only be owned by ApplicationModules or other VariableGroups. */
      VariableGroup(EntityOwner *owner, const std::string &name, const std::string &description,
             bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

      /** Default constructor: Allows late initialisation of VariableGroups (e.g. when creating arrays of
       *  VariableGroups).
       * 
       *  This construtor also has to be here to mitigate a bug in gcc. It is needed to allow constructor
       *  inheritance of modules owning other modules. This constructor will not actually be called then.
       *  See this bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
      VariableGroup() : Module(nullptr, "invalid", "invalid VariableGroup") {}

      /** Destructor */
      virtual ~VariableGroup() {};

      /** Move operation with the move constructor */
      VariableGroup(VariableGroup &&other) :
      Module(std::move(other))
      {}
      
      /** Inherit assignment */
      using Module::operator=;

      ModuleType getModuleType() const override { return ModuleType::VariableGroup; }

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_GROUP_H */
