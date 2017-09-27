/*
* ArrayAccessor.h
*
*  Created on: Jun 07, 2016
*      Author: Martin Hierholzer
*/

#ifndef CHIMERATK_ARRAY_ACCESSOR_H
#define CHIMERATK_ARRAY_ACCESSOR_H

#include <string>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <mtca4u/OneDRegisterAccessor.h>

#include "VariableNetworkNode.h"
#include "Profiler.h"

namespace ChimeraTK {

  /** Accessor for array variables (i.e. vectors). Note for users: Use the convenience classes
  *  ArrayPollInput, ArrayPushInput, ArrayOutput instead of this class directly. */
  template< typename UserType >
  class ArrayAccessor :  public mtca4u::OneDRegisterAccessor<UserType> {
    public:

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

      /** Convert into VariableNetworkNode */
      operator VariableNetworkNode() {
        return node;
      }
      
      /** Connect with other node */
      VariableNetworkNode operator>>(const VariableNetworkNode &otherNode) {
        return node >> otherNode;
      }
      /** Replace with other ScalarRegisterAccessor */
      void replace(ArrayAccessor<UserType> &&other) {
        operator=(std::move(other));
      }
      
      /** Move constructor */
      ArrayAccessor(ArrayAccessor<UserType> &&other) {
        operator=(std::move(other));
      }

      void replace(const mtca4u::NDRegisterAccessorBridge<UserType> &newAccessor) = delete;

      /** Move-assignment operator as an alternative for replace where applicable. This is needed to allow late
       *  initialisation of ApplicationModules using ArrayAccessors */
      ArrayAccessor<UserType>& operator=(ArrayAccessor<UserType> &&other) {
        assert(this->_impl == nullptr);
        node = other.node;    // just copies the pointer, but other will be destroyed right after this move constructor
        other.node = VariableNetworkNode();
        node.setAppAccessorPointer(this);
        // Note: the accessor is registered by the VariableNetworkNode, so we don't have to re-register. Setting the
        // owner of the other accessor to nullptr will prevent unregistering the accessor.
        return *this;
      }

      using mtca4u::OneDRegisterAccessor<UserType>::operator=;

      ~ArrayAccessor() {
        if(getOwner() != nullptr) getOwner()->unregisterAccessor(node);
      }
      
      /** Add a tag. Valid names for tags only contain alpha-numeric characters (i.e. no spaces and no special
       *  characters). */
      void addTag(const std::string &tag) {
        node.addTag(tag);
      }
      
      void read() {
        Profiler::stopMeasurement();
        mtca4u::OneDRegisterAccessor<UserType>::read();
        Profiler::startMeasurement();
      }

      EntityOwner* getOwner() const { return node.getOwningModule(); }

  protected:

      ArrayAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          size_t nElements, UpdateMode mode, const std::string &description,
          const std::unordered_set<std::string> &tags={})
        : node(owner, this, name, direction, unit, nElements, mode, description, &typeid(UserType), tags)
      {
        owner->registerAccessor(*this);
      }
      
      /** Default constructor creates a dysfunctional accessor (to be assigned with a real accessor later) */
      ArrayAccessor() {}
    
      VariableNetworkNode node;

  };

  /** Convenience class for input array accessors with UpdateMode::push */
  template< typename UserType >
  struct ArrayPushInput : public ArrayAccessor<UserType> {
    ArrayPushInput(Module *owner, const std::string &name, std::string unit, size_t nElements,
                   const std::string &description, const std::unordered_set<std::string> &tags={})
    : ArrayAccessor<UserType>(owner, name, VariableDirection::consuming, unit, nElements, UpdateMode::push,
                              description, tags)
    {}
    ArrayPushInput() : ArrayAccessor<UserType>() {}
    using ArrayAccessor<UserType>::operator=;
  };

  /** Convenience class for input array accessors with UpdateMode::poll */
  template< typename UserType >
  struct ArrayPollInput : public ArrayAccessor<UserType> {
    ArrayPollInput(Module *owner, const std::string &name, std::string unit, size_t nElements,
                   const std::string &description, const std::unordered_set<std::string> &tags={})
    : ArrayAccessor<UserType>(owner, name, VariableDirection::consuming, unit, nElements, UpdateMode::poll,
                              description, tags)
    {}
    ArrayPollInput() : ArrayAccessor<UserType>() {}
    void doReadTransfer() override { this->doReadTransferLatest(); }
    void read() { this->readLatest(); }
    using ArrayAccessor<UserType>::operator=;
  };

  /** Convenience class for output array accessors (always UpdateMode::push) */
  template< typename UserType >
  struct ArrayOutput : public ArrayAccessor<UserType> {
    ArrayOutput(Module *owner, const std::string &name, std::string unit, size_t nElements,
                const std::string &description, const std::unordered_set<std::string> &tags={})
    : ArrayAccessor<UserType>(owner, name, VariableDirection::feeding, unit, nElements, UpdateMode::push,
                              description, tags)
    {}
    ArrayOutput() : ArrayAccessor<UserType>() {}
    using ArrayAccessor<UserType>::operator=;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ARRAY_ACCESSOR_H */
