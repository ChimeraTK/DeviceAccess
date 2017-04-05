/*
 * Module.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_H
#define CHIMERATK_MODULE_H

#include "VariableNetworkNode.h"
#include "EntityOwner.h"

namespace ChimeraTK {

  /** Base class for ApplicationModule, DeviceModule and ControlSystemModule, to have a common interface for these
   *  module types. */
  class Module : public EntityOwner {

    public:

      /** Constructor: register the module with its owner */
      Module(EntityOwner *owner, const std::string &name, const std::string &description,
             bool eliminateHierarchy=false);
      
      /** Default constructor: Allows late initialisation of modules (e.g. when creating arrays of modules).
       * 
       *  This construtor also has to be here to mitigate a bug in gcc. It is needed to allow constructor
       *  inheritance of modules owning other modules. This constructor will not actually be called then.
       *  See this bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
      Module() : EntityOwner(nullptr, "invalid", "invalid module") {}

      /** Destructor */
      virtual ~Module();

      /** Prepare the execution of the module. */
      virtual void prepare() {};

      /** Execute the module. */
      virtual void run() {};

      /** Terminate the module. Must be called before destruction, if run() was called previously. */
      virtual void terminate() {};

      /** Function call operator: Return VariableNetworkNode of the given variable name */
      virtual VariableNetworkNode operator()(const std::string& variableName) = 0;

      /** Subscript operator: Return sub-module of the given name */
      virtual Module& operator[](const std::string& moduleName) = 0;
      
      /** ">=" operator: Connect the entire module into another module. All variables inside this module and all
        * submodules are connected to the right-hand-side module. All variables and submodules must have an equally
        * named and typed counterpart in the right-hand-side module (or the right-hand-side module allows creation of
        * such entities, as in case of a ControlSystemModule). The right-hand-side module may contain additional
        * variables or submodules, which are ignored. */
      Module& operator>=(Module &rhs);
      
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_H */
