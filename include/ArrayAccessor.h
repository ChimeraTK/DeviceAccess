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

  /** Accessor for array variables (i.e. vectors). Note for users: Preferrably use the convenience classes
   *  ArrayPollInput, ArrayPushInput, ArrayOutput instead of this class directly. */
  template< typename UserType >
  class ArrayAccessor :  public mtca4u::OneDRegisterAccessor<UserType> {
    public:
      ArrayAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          size_t nElements, UpdateMode mode, const std::string &description)
      : node(this, name, direction, unit, nElements, mode, description, &typeid(UserType))
      {
        owner->registerAccessor(*this);
      }
      
      /** Default constructor creates a dysfunction accessor (to be assigned with a real accessor later) */
      ArrayAccessor() {}

      /** Convert into VariableNetworkNode */
      operator VariableNetworkNode() {
        return node;
      }
      
      /** Connect with other node */
      VariableNetworkNode operator>>(const VariableNetworkNode &otherNode) {
        return node >> otherNode;
      }

      using mtca4u::OneDRegisterAccessor<UserType>::operator=;
      
  protected:
    
      VariableNetworkNode node;

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
