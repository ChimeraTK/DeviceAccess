/*
* InversionOfControlAccessor.h
*
*  Created on: Sep 28, 2017
*      Author: Martin Hierholzer
*/

#ifndef CHIMERATK_INVERSION_OF_CONTROL_ACCESSOR_H
#define CHIMERATK_INVERSION_OF_CONTROL_ACCESSOR_H

#include <string>

#include <boost/smart_ptr/shared_ptr.hpp>

#include "Module.h"
#include "VariableNetworkNode.h"

namespace ChimeraTK {

  /** Adds features required for inversion of control to an accessor. This is needed for both the ArrayAccessor and
   *  the ScalarAccessor classes, thus it uses a CRTP. */
  template< typename Derived >
  class InversionOfControlAccessor {

    public:

      /** Unregister at its owner when deleting */
      ~InversionOfControlAccessor() {
        if(getOwner() != nullptr) getOwner()->unregisterAccessor(node);
      }

      /** Change meta data (name, unit, description and optionally tags). This function may only be used on
       *  Application-type nodes. If the optional argument tags is omitted, the tags will not be changed. To clear the
       *  tags, an empty set can be passed. */
      void setMetaData(const std::string &name, const std::string &unit, const std::string &description) {
        node.setMetaData(name, unit, description);
      }
      void setMetaData(const std::string &name, const std::string &unit, const std::string &description,
                       const std::unordered_set<std::string> &tags) {
        node.setMetaData(name, unit, description, tags);
      }
      
      /** Add a tag. Valid names for tags only contain alpha-numeric characters (i.e. no spaces and no special
       *  characters). */
      void addTag(const std::string &tag) {
        node.addTag(tag);
      }
      
      /** Add multiple tags. Valid names for tags only contain alpha-numeric characters (i.e. no spaces and no special
       *  characters). */
      void addTags(const std::unordered_set<std::string> &tags) {
        for(auto &tag : tags) node.addTag(tag);
      }

      /** Convert into VariableNetworkNode */
      operator VariableNetworkNode() {
        return node;
      }
      
      /** Connect with other node */
      VariableNetworkNode operator>>(const VariableNetworkNode &otherNode) {
        return node >> otherNode;
      }

      /** Replace with other accessor */
      void replace(Derived &&other) {
        assert(static_cast<Derived*>(this)->_impl == nullptr && other._impl == nullptr);
        if(getOwner() != nullptr) getOwner()->unregisterAccessor(node);
        node = other.node;    // just copies the pointer, but other will be destroyed right after this move constructor
        other.node = VariableNetworkNode();
        node.setAppAccessorPointer(static_cast<Derived*>(this));
        // Note: the accessor is registered by the VariableNetworkNode, so we don't have to re-register.
      }

      /** Return the owning module */
      EntityOwner* getOwner() const { return node.getOwningModule(); }

  protected:

      InversionOfControlAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          size_t nElements, UpdateMode mode, const std::string &description, const std::type_info* valueType,
          const std::unordered_set<std::string> &tags={})
        : node(owner, static_cast<Derived*>(this), name, direction, unit, nElements, mode, description, valueType, tags)
      {
        static_assert(std::is_base_of<InversionOfControlAccessor<Derived>, Derived>::value,
                      "InversionOfControlAccessor<> must be used in a curiously recurring template pattern!");
        owner->registerAccessor(node);
      }

      /** Default constructor creates a dysfunctional accessor (to be assigned with a real accessor later) */
      InversionOfControlAccessor() {}
    
      VariableNetworkNode node;

  };

}

#endif /* CHIMERATK_INVERSION_OF_CONTROL_ACCESSOR_H */
