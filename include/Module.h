/*
 * Module.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_H
#define CHIMERATK_MODULE_H

#include "EntityOwner.h"
#include "VariableNetworkNode.h"
#include <ChimeraTK/ReadAnyGroup.h>
#include <ChimeraTK/TransferElement.h>

namespace ChimeraTK {

  /** Base class for ApplicationModule, DeviceModule and ControlSystemModule, to
   * have a common interface for these module types. */
  class Module : public EntityOwner {
   public:
    /** Constructor: Create Module by the given name with the given description and register it with its owner. The
     *  hierarchy will be modified according to the hierarchyModifier (when VirtualModules are created e.g. in
     *  findTag()). The specified list of tags will be added to all elements directly or indirectly owned by this
     *  instance. */
    Module(EntityOwner* owner, const std::string& name, const std::string& description,
        HierarchyModifier hierarchyModifier = HierarchyModifier::none,
        const std::unordered_set<std::string>& tags = {});

    /** Deprecated form of the constructor. Use the new signature instead. */
    Module(EntityOwner* owner, const std::string& name, const std::string& description, bool eliminateHierarchy,
        const std::unordered_set<std::string>& tags = {});

    /** Default constructor: Allows late initialisation of modules (e.g. when
     * creating arrays of modules).
     *
     *  This construtor also has to be here to mitigate a bug in gcc. It is needed
     * to allow constructor inheritance of modules owning other modules. This
     * constructor will not actually be called then. See this bug report:
     * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67054 */
    Module() : EntityOwner(), _owner{nullptr} {}

    /** Destructor */
    virtual ~Module();

    /** Move constructor */
    Module(Module&& other) { operator=(std::move(other)); }

    /** Move assignment operator */
    Module& operator=(Module&& other);

    /** Prepare the execution of the module. This function is called before any module is started (including internal
     *  modules like FanOuts) and before the initial values of the variables are pushed into the queues. Reading and
     *  writing variables at this point may result in undefined behaviour. */
    virtual void prepare(){};

    /** Execute the module. */
    virtual void run();

    /** Terminate the module. Must/will be called before destruction, if run() was
     * called previously. */
    virtual void terminate(){};

    /** Create a ChimeraTK::ReadAnyGroup for all readable variables in this
     * Module. */
    ChimeraTK::ReadAnyGroup readAnyGroup();

    /** Read all readable variables in the group. If there are push-type variables in the group, this call will block
     *  until all of the variables have received an update. All push-type variables are read first, the poll-type
     *  variables are therefore updated with the latest values upon return.
     *  includeReturnChannels determines whether return channels of *OutputRB accessors are included in the read. */
    void readAll(bool includeReturnChannels = false);

    /** Just call readNonBlocking() on all readable variables in the group.
     *  includeReturnChannels determines whether return channels of *OutputRB accessors are included in the read. */
    void readAllNonBlocking(bool includeReturnChannels = false);

    /** Just call readLatest() on all readable variables in the group.
     *  includeReturnChannels determines whether return channels of *OutputRB accessors are included in the read. */
    void readAllLatest(bool includeReturnChannels = false);

    /** Just call write() on all writable variables in the group.
     *  includeReturnChannels determines whether return channels of *InputWB accessors are included in the write. */
    void writeAll(bool includeReturnChannels = false);

    /** Just call writeDestructively() on all writable variables in the group.
     *  includeReturnChannels determines whether return channels of *InputWB accessors are included in the write. */
    void writeAllDestructively(bool includeReturnChannels = false);

    /** Function call operator: Return VariableNetworkNode of the given variable
     * name */
    virtual VariableNetworkNode operator()(const std::string& variableName) const = 0;

    /** Subscript operator: Return sub-module of the given name. Hierarchies will
     * already be eliminated, if requested. Thus the returned reference will not
     * point to any user-defined object but to a VirtualModule containing the
     * variable structure. */
    virtual Module& operator[](const std::string& moduleName) const = 0;

    /** Convenience function which works similar as the subscript operator []. In contrast to the operator, this
     *  function allows to specify directly the name of a sub-submodule on a deeper hierarchy level. The call to
     *  submodule("livingroom/temperature") is equivalent to ["livingroom"]["temperature"]. */
    Module& submodule(const std::string& moduleName) const;

    /** Return the virtual version of this module and its sub-modules, i.e.
     * eliminate hierarchies where requested and apply other dynamic model
     * changes. */
    virtual const Module& virtualise() const = 0;
    virtual void defineConnections(){};

    /**
     * Connect the entire module into another module. All variables inside this
     * module and all submodules are connected to the target module. All variables
     * and submodules must have an equally named and typed counterpart in the
     * target module (or the target module allows creation of such entities, as in
     * case of a ControlSystemModule). The target module may contain additional
     * variables or submodules, which are ignored.
     *
     * If an optional trigger node is specified, this trigger node is applied to
     * all poll-type output variables of the target module, which are being
     * connected during this operation, if the corresponding variable in this
     * module is push-type.
     */
    virtual void connectTo(const Module& target, VariableNetworkNode trigger = {}) const = 0;

    std::string getQualifiedName() const override {
      return ((_owner != nullptr) ? _owner->getQualifiedName() : "") + "/" + _name;
    }

    virtual std::string getVirtualQualifiedName() const;

    std::string getFullDescription() const override {
      if(_owner == nullptr) return _description;
      auto ownerDescription = _owner->getFullDescription();
      if(ownerDescription == "") return _description;
      if(_description == "") return ownerDescription;
      return ownerDescription + " - " + _description;
    }

    /** Set a new owner. The caller has to take care himself that the Module gets
     * unregistered with the old owner
     *  and registered with the new one. Do not use in user code! */
    void setOwner(EntityOwner* newOwner) { _owner = newOwner; }

    EntityOwner* getOwner() const { return _owner; }

    /**
     * Explcitly add accept() method so that we can distinguish between a Module
     and an EntityOwner in the Visitor.

     */
    void accept(Visitor<Module>& visitor) const { visitor.dispatch(*this); }

    VersionNumber getCurrentVersionNumber() const override { return _owner->getCurrentVersionNumber(); }
    void setCurrentVersionNumber(VersionNumber version) override { _owner->setCurrentVersionNumber(version); }

    DataValidity getDataValidity() const override { return _owner->getDataValidity(); }
    void incrementDataFaultCounter() override { _owner->incrementDataFaultCounter(); }
    void decrementDataFaultCounter() override { _owner->decrementDataFaultCounter(); }

   protected:
    /** Owner of this instance */
    EntityOwner* _owner{nullptr};
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_H */
