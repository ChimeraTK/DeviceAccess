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

namespace ChimeraTK {

  /** Accessor for array variables (i.e. vectors). Note for users: Use the convenience classes
  *  ArrayPollInput, ArrayPushInput, ArrayOutput instead of this class directly. */
  template< typename UserType >
  class ArrayAccessor :  public mtca4u::OneDRegisterAccessor<UserType> {
    public:

      /** Convert into VariableNetworkNode */
      operator VariableNetworkNode() {
        return node;
      }
      
      /** Connect with other node */
      VariableNetworkNode operator>>(const VariableNetworkNode &otherNode) {
        return node >> otherNode;
      }
      /** Replace with other ScalarRegisterAccessor */
      void replace(const ArrayAccessor<UserType> &newAccessor) {
        if(_owner != newAccessor._owner && _owner != nullptr) {
          _owner->unregisterAccessor(node);
        }
        mtca4u::NDRegisterAccessorBridge<UserType>::replace(newAccessor);
        node = VariableNetworkNode(this, newAccessor.node.getName(), newAccessor.node.getDirection(), newAccessor.node.getUnit(),
                                  newAccessor.node.getNumberOfElements(), newAccessor.node.getMode(), newAccessor.node.getDescription(),
                                  &newAccessor.node.getValueType());
        if(_owner != newAccessor._owner) {
          _owner = newAccessor._owner;
          if(_owner != nullptr) _owner->registerAccessor(node);
        }
      }

      void replace(const NDRegisterAccessorBridge<UserType> &newAccessor) = delete;

      /** Move-assignment operator as an alternative for replace where applicable. This is needed to allow late
       *  initialisation of ApplicationModules using ArrayAccessors */
      ArrayAccessor<UserType>& operator=(ArrayAccessor<UserType> &&rhs) {
        mtca4u::NDRegisterAccessorBridge<UserType>::replace(rhs);
        node.pdata = rhs.node.pdata;
        node.pdata->appNode = this;
        return *this;
      }

      using mtca4u::OneDRegisterAccessor<UserType>::operator=;

      ~ArrayAccessor() {
        if(_owner != nullptr) _owner->unregisterAccessor(*this);
      }
      
  protected:
      ArrayAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          size_t nElements, UpdateMode mode, const std::string &description)
        : node(this, name, direction, unit, nElements, mode, description, &typeid(UserType)), _owner(owner)
      {
        owner->registerAccessor(*this);
      }
      
      /** Default constructor creates a dysfunction accessor (to be assigned with a real accessor later) */
      ArrayAccessor() {}
    
      VariableNetworkNode node;

      Module *_owner{nullptr};

  };

  /** Convenience class for input array accessors with UpdateMode::push */
  template< typename UserType >
  struct ArrayPushInput : public ArrayAccessor<UserType> {
    ArrayPushInput(Module *owner, const std::string &name, std::string unit, size_t nElements, const std::string &description)
    : ArrayAccessor<UserType>(owner, name, VariableDirection::consuming, unit, nElements, UpdateMode::push, description)
    {}
    ArrayPushInput() : ArrayAccessor<UserType>() {}
    using ArrayAccessor<UserType>::operator=;
  };

  /** Convenience class for input array accessors with UpdateMode::poll */
  template< typename UserType >
  struct ArrayPollInput : public ArrayAccessor<UserType> {
    ArrayPollInput(Module *owner, const std::string &name, std::string unit, size_t nElements, const std::string &description)
    : ArrayAccessor<UserType>(owner, name, VariableDirection::consuming, unit, nElements, UpdateMode::poll, description)
    {}
    ArrayPollInput() : ArrayAccessor<UserType>() {}
    void doReadTransfer() override { this->doReadTransferNonBlocking(); }
    void read() { this->readNonBlocking(); }
    using ArrayAccessor<UserType>::operator=;
  };

  /** Convenience class for output array accessors (always UpdateMode::push) */
  template< typename UserType >
  struct ArrayOutput : public ArrayAccessor<UserType> {
    ArrayOutput(Module *owner, const std::string &name, std::string unit, size_t nElements, const std::string &description)
    : ArrayAccessor<UserType>(owner, name, VariableDirection::feeding, unit, nElements, UpdateMode::push, description)
    {}
    ArrayOutput() : ArrayAccessor<UserType>() {}
    using ArrayAccessor<UserType>::operator=;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ARRAY_ACCESSOR_H */
