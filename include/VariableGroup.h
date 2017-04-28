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

  class VariableGroup : public Module {

    public:
      
      using Module::Module;

      /** Destructor */
      virtual ~VariableGroup() {};
      
      /** Move operation with the assignment operator
          @todo should be in the base class!? */
      VariableGroup& operator=(VariableGroup &&rhs) {
        _name = std::move(rhs._name);
        _owner = std::move(rhs._owner);
        accessorList = std::move(rhs.accessorList);
        moduleList = std::move(rhs.moduleList);
        return *this;
      }
      
      VariableGroup& operator=(VariableGroup &rhs) = delete;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_GROUP_H */
