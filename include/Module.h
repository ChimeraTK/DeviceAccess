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

      /** Constructor: register the module with its owner. If eliminateHierarchy is true, the hierarchy level
       *  introduced by this module will be eliminated from the "dynamic" data model (see
       *  EntityOwner::setEliminateHierarchy()). The tags given as the last argument are added to all variables
       *  in this module recursively (see EntityOwner::addTag()). */
      Module(EntityOwner *owner, const std::string &name, const std::string &description,
             bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

      /** Default constructor: Allows late initialisation of modules (e.g. when creating arrays of modules).
       * 
       *  This construtor also has to be here to mitigate a bug in gcc. It is needed to allow constructor
       *  inheritance of modules owning other modules. This constructor will not actually be called then.
       *  See this bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
      Module() : EntityOwner(nullptr, "invalid", "invalid module") {}

      /** Destructor */
      virtual ~Module();
      
      /** Move constructor */
      Module(Module &&other)
      : EntityOwner(std::move(other))
      {
        _owner->unregisterModule(&other);
        _owner->registerModule(this);
        other._owner = nullptr;
      }

      Module& operator=(Module &&other) {
        _name = std::move(other._name);
        _description = std::move(other._description);
        _owner = other._owner;
        _owner->unregisterModule(&other);
        _owner->registerModule(this);
        other._owner = nullptr;
        accessorList = std::move(other.accessorList);
        moduleList = std::move(other.moduleList);
        _eliminateHierarchy = other._eliminateHierarchy;
        _tags = std::move(other._tags);
        return *this;
      }
      
      /** Delete other assignment operators */
      Module& operator=(Module &other) = delete;
      Module& operator=(const Module &other) = delete;


      /** Prepare the execution of the module. This function is called before any module is started (including internal
       *  modules like FanOuts) and before the initial values of the variables are pushed into the queues. */
      virtual void prepare() {};

      /** Execute the module. */
      virtual void run() {};

      /** Terminate the module. Must/will be called before destruction, if run() was called previously. */
      virtual void terminate() {};
      
      /** Wait for receiving an update for any of the push-type variables in the group. Any poll-type variables are
       *  read after receiving the update. If no push-type variables are in the group, this function will just read
       *  all variables. The returned TransferElement will be the push-type variable which has been updated. */
      boost::shared_ptr<mtca4u::TransferElement> readAny();

      /** Read all readable variables in the group. If there are push-type variables in the group, this call will block
       *  until all of the variables have received an update. All push-type variables are read first, the poll-type
       *  variables are therefore updated with the latest values upon return. */
      void readAll();

      /** Just call readNonBlocking() on all readable variables in the group. */
      void readAllNonBlocking();

      /** Just call readLatest() on all readable variables in the group. */
      void readAllLatest();

      /** Just call write() on all writable variables in the group. */
      void writeAll();

      /** Function call operator: Return VariableNetworkNode of the given variable name */
      virtual VariableNetworkNode operator()(const std::string& variableName) const;

      /** Subscript operator: Return sub-module of the given name */
      virtual Module& operator[](const std::string& moduleName) const;
      
      /** Connect the entire module into another module. All variables inside this module and all
        * submodules are connected to the target module. All variables and submodules must have an equally
        * named and typed counterpart in the target module (or the target module allows creation of
        * such entities, as in case of a ControlSystemModule). The target module may contain additional
        * variables or submodules, which are ignored.
        * 
        * If an optional trigger node is specified, this trigger node is applied to all poll-type output variables
        * of the target module, which are being connected during this operation, if the corresponding variable
        * in this module is push-type.
        *
        * Note: This function is a template to allow special modules to provide overridden () and >> operators with
        * a changed return type of the () operator. */
      template<typename MODULE>
      void connectTo(const MODULE &target, VariableNetworkNode trigger={}) const;
      
  };
  
/*********************************************************************************************************************/

  template<typename MODULE>
  void Module::connectTo(const MODULE &target, VariableNetworkNode trigger) const {
    
    // connect all direct variables of this module to their counter-parts in the right-hand-side module
    for(auto variable : getAccessorList()) {
      if(variable.getDirection() == VariableDirection::feeding) {
        variable >> target(variable.getName());
      }
      else {
        // use trigger?
        if(trigger != VariableNetworkNode() && target(variable.getName()).getMode() == UpdateMode::poll
                                            && variable.getMode() == UpdateMode::push ) {
          target(variable.getName()) [ trigger ] >> variable;
        }
        else {
          target(variable.getName()) >> variable;
        }
      }
    }
    
    // connect all sub-modules to their couter-parts in the right-hand-side module
    for(auto submodule : getSubmoduleList()) {
      submodule->connectTo(target[submodule->getName()], trigger);
    }
    
  }

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_H */
