/*
 * EntityOwner.h
 *
 *  Created on: Nov 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_ENTITY_OWNER_H
#define CHIMERATK_ENTITY_OWNER_H

#include <string>
#include <list>

#include "VariableNetworkNode.h"

namespace ChimeraTK {
  
  class AccessorBase;
  class Module;

  /** Base class for owners of other EntityOwners (e.g. Modules) and Accessors.
    * @todo Rename this class to "Owner" and make it more generic. It should basically just implement the
    * "Composite Pattern". The classes AccessorBase, Module and Owner should have a common base class called
    * "Component".
    */
  class EntityOwner {

    public:

      /** Constructor: register the EntityOwner with its owner */
      EntityOwner(EntityOwner *owner, const std::string &name);
      
      /** Virtual destructor to make the type polymorphic */
      virtual ~EntityOwner();
      
      /** Get the name of the module instance */
      const std::string& getName() const { return _name; }
      
      /** Obtain the list of accessors/variables directly associated with this instance */
      std::list<VariableNetworkNode>& getAccessorList() { return accessorList; }
      
      /** Obtain the list of submodules associated with this instance */
      const std::list<Module*>& getSubmoduleList() const { return moduleList; }
      
      /** Obtain the list of accessors/variables associated with this instance and any submodules */
      std::list<VariableNetworkNode> getAccessorListRecursive();
      
      /** Obtain the list of submodules associated with this instance and any submodules */
      std::list<Module*> getSubmoduleListRecursive();

      /** Called inside the constructor of Accessor: adds the accessor to the list */
      void registerAccessor(VariableNetworkNode accessor) {
        accessorList.push_back(accessor);
      }

      /** Called inside the destructor of Accessor: removes the accessor from the list */
      void unregisterAccessor(VariableNetworkNode accessor) {
        accessorList.remove(accessor);
      }
      
      /** Register another module as a sub-mdoule. Will be called automatically by all modules in their constructors. */
      void registerModule(Module* module);
      
      /** Unregister another module as a sub-mdoule. Will be called automatically by all modules in their destructors. */
      void unregisterModule(Module* module);

  protected:
    
      /** The name of this instance */
      std::string _name;
      
      /** Owner of this instance */
      EntityOwner *_owner{nullptr};
      
      /** List of accessors owned by this instance */
      std::list<VariableNetworkNode> accessorList;

      /** List of modules owned by this instance */
      std::list<Module*> moduleList;
      
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ENTITY_OWNER_H */

