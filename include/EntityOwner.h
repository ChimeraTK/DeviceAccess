/*
 * EntityOwner.h
 *
 *  Created on: Nov 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_ENTITY_OWNER_H
#define CHIMERATK_ENTITY_OWNER_H

#include <list>
#include <string>

#include "VariableNetworkNode.h"
#include "Flags.h"

namespace ChimeraTK {

  class AccessorBase;
  class Module;
  class VirtualModule;

  /** Base class for owners of other EntityOwners (e.g. Modules) and Accessors.
   * @todo Rename this class to "Owner" and make it more generic. It should
   * basically just implement the "Composite Pattern". The classes AccessorBase,
   * Module and Owner should have a common base class called "Component".
   */
  class EntityOwner {
   public:
    /** Constructor: Create EntityOwner by the given name with the given description. The hierarchy will be modified
     *  according to the hierarchyModifier (when VirtualModules are created e.g. in findTag()). The specified list of
     *  tags will be added to all elements directly or indirectly owned by this instance. */
    EntityOwner(const std::string& name, const std::string& description,
        HierarchyModifier hierarchyModifier = HierarchyModifier::none,
        const std::unordered_set<std::string>& tags = {});

    /** Deprecated form of the constructor. Use the new signature instead. */
    EntityOwner(const std::string& name, const std::string& description, bool eliminateHierarchy,
        const std::unordered_set<std::string>& tags = {});

    /** Default constructor just for late initialisation */
    EntityOwner()
    : _name("**INVALID**"), _description("Invalid EntityOwner created by default constructor just "
                                         "as a place holder") {}

    /** Virtual destructor to make the type polymorphic */
    virtual ~EntityOwner();

    /** Move constructor */
    EntityOwner(EntityOwner&& other) { operator=(std::move(other)); }
    EntityOwner(const EntityOwner& other) = delete;

    /** Move assignment operator */
    EntityOwner& operator=(EntityOwner&& other);
    EntityOwner& operator=(const EntityOwner& other) = delete;

    /** Get the name of the module instance */
    const std::string& getName() const { return _name; }

    /** Get the fully qualified name of the module instance, i.e. the name
     * containing all module names further up in the hierarchy. */
    virtual std::string getQualifiedName() const = 0;

    /** Get the decription of the module instance */
    const std::string& getDescription() const { return _description; }

    /** Obtain the full description including the full description of the owner.
     */
    virtual std::string getFullDescription() const = 0;

    /** Obtain the list of accessors/variables directly associated with this
     * instance */
    virtual std::list<VariableNetworkNode> getAccessorList() const { return accessorList; }

    /** Obtain the list of submodules associated with this instance */
    virtual std::list<Module*> getSubmoduleList() const { return moduleList; }

    /** Obtain the list of accessors/variables associated with this instance and
     * any submodules */
    std::list<VariableNetworkNode> getAccessorListRecursive();

    /** Obtain the list of submodules associated with this instance and any
     * submodules */
    std::list<Module*> getSubmoduleListRecursive();

    /** Check whether a submodule exists by the given name (not taking into
     * account eliminated hierarchies etc.) */
    bool hasSubmodule(const std::string& name) const;

    /** Get a submodule by the given name (not taking into account eliminated
     * hierarchies etc.) */
    Module* getSubmodule(const std::string& name) const;

    /** Return a VirtualModule containing the part of the tree structure matching
     * the given tag. The resulting VirtualModule might have virtual sub-modules,
     * if this EntityOwner contains sub-EntityOwners with entities matchting the
     * tag. "tag" is interpreted as a regular expression (see std::regex_match).
     */
    VirtualModule findTag(const std::string& tag) const;

    /** Return a VirtualModule containing the part of the tree structure not
     * matching the given tag. This is the negation of findTag(), this function
     * will keep those variables which findTag() would remove from the tree.
     * Again, "tag" is interpreted as a regular expression (see std::regex_match).
     */
    VirtualModule excludeTag(const std::string& tag) const;

    /** Called inside the constructor of Accessor: adds the accessor to the list
     */
    void registerAccessor(VariableNetworkNode accessor) {
      for(auto& tag : _tags) accessor.addTag(tag);
      accessorList.push_back(accessor);
    }

    /** Called inside the destructor of Accessor: removes the accessor from the
     * list */
    void unregisterAccessor(VariableNetworkNode accessor) { accessorList.remove(accessor); }

    /** Register another module as a sub-mdoule. Will be called automatically by
     * all modules in their constructors. If addTags is set to false, the tags of
     * this EntityOwner will not be set to the module being registered. This is
     * e.g. used in the move-constructor of Module to prevent from altering the
     * tags in the move operation. */
    void registerModule(Module* module, bool addTags = true);

    /** Unregister another module as a sub-mdoule. Will be called automatically by
     * all modules in their destructors. */
    void unregisterModule(Module* module);

    /** Add a tag to all Application-type nodes inside this group. It will recurse
     * into any subgroups. See VariableNetworkNode::addTag() for additional
     * information about tags. */
    void addTag(const std::string& tag);

    /** Note: this function is deprectated. Use the constructor parameter instead. If this is not sufficient, write a
     *  feature request for a function to set the HierarchyModifier.
     *
     * Eliminate the level of hierarchy represented by this EntityOwner. This is
     * e.g. used when building the hierarchy of VirtualModules in findTag().
     * Eliminating one level of hierarchy will make all childs of that hierarchy
     * level to appear as if there were direct childs of the next higher hierarchy
     * level. If e.g. there is a variable on the third level "A.B.C" and one
     * selects to eliminate the second level of hierarchy (e.g. calls
     *  B.eliminateHierarchy()), the structure would look like "A.C". This of
     * course only affects the "dynamic" data
     *  model, while the static C++ model is fixed at compile time. */
    void setEliminateHierarchy() { _hierarchyModifier = HierarchyModifier::hideThis; }

    /** Returns the flag whether this level of hierarchy should be eliminated. It returns true
        if hiding the hierarchy is required by the hierarchy modifier (HierarchyModifier::hideThis or HierarchyModifier::oneUpAndHide) */
    bool getEliminateHierarchy() const;

    /** Returns the hierarchy modifier of this entity. FIXE: One of those useless code comments.
     */
    HierarchyModifier getHierarchyModifier() const { return _hierarchyModifier; }

    /** Create a VirtualModule which contains all variables of this EntityOwner in
     * a flat hierarchy. It will recurse
     *  through all sub-modules and add all found variables directly to the
     * VirtualModule. */
    VirtualModule flatten();

    void accept(Visitor<EntityOwner>& visitor) const { visitor.dispatch(*this); }

    /** Print the full hierarchy to stdout. */
    void dump(const std::string& prefix = "") const;

    /** Create Graphviz dot graph and write to file. The graph will contain the
     * full hierarchy of modules and variables below (and including) this module.
     * Each variable will also show which tags are attached to it. ModuleGroups
     * will be drawn with a double line, ApplicationModules with a bold line.
     * Hierarchies which will be eliminated in the dynamic information model are
     * shown with a dotted line. */
    void dumpGraph(const std::string& fileName = "graph.dot") const;

    /** Create a Graphiz dot graph similar to the one created with dumpGraph, but
     * just show the modules and not the
     *  variables. This allows to get an overview over more complex applications.
     */
    void dumpModuleGraph(const std::string& fileName = "graph.dot") const;

    enum class ModuleType { ApplicationModule, ModuleGroup, VariableGroup, ControlSystem, Device, Invalid };

    /** Return the module type of this module, or in case of a VirtualModule the
     * module type this VirtualModule was derived from. */
    virtual ModuleType getModuleType() const = 0;

    /** Return the current version number which has been received with the last
     * push-type read operation. */
    virtual VersionNumber getCurrentVersionNumber() const = 0;

    /** Set the current version number. This function is called by the push-type
     * input accessors in their read functions. */
    virtual void setCurrentVersionNumber(VersionNumber versionNumber) = 0;

    /** Return the data validity flag. If any This function will be called by all output accessors in their write
     *  functions. */
    virtual DataValidity getDataValidity() const = 0;

    /** Set the data validity flag to fault and increment the fault counter. This function will be called by all input
     *  accessors when receiving the a faulty update if the previous update was ok. The caller of this function must
     *  ensure that calls to this function are paired to a subsequent call to decrementDataFaultCounter(). */
    virtual void incrementDataFaultCounter() = 0;

    /** Decrement the fault counter and set the data validity flag to ok if the counter has reached 0. This function
     *  will be called by all input accessors when receiving the an ok update if the previous update was faulty. The
     *  caller of this function must ensure that calles to this function are paired to a previous call to
     *  incrementDataFaultCounter(). */
    virtual void decrementDataFaultCounter() = 0;

   protected:
    /** Add the part of the tree structure matching the given tag to a
     * VirtualModule. Users normally will use findTag() instead. "tag" is
     * interpreted as a regular expression (see std::regex_match). */
    void findTagAndAppendToModule(VirtualModule& virtualParent, const std::string& tag, bool eliminateAllHierarchies,
        bool eliminateFirstHierarchy, bool negate, VirtualModule& root, VirtualModule* virtualGrandparent) const;

    /** The name of this instance */
    std::string _name;

    /** The description of this instance */
    std::string _description;

    /** List of accessors owned by this instance */
    std::list<VariableNetworkNode> accessorList;

    /** List of modules owned by this instance */
    std::list<Module*> moduleList;

    /** Hierarchy modifier flag */
    HierarchyModifier _hierarchyModifier{HierarchyModifier::none};

    /** List of tags to be added to all accessors and modules inside this module
     */
    std::unordered_set<std::string> _tags;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ENTITY_OWNER_H */
