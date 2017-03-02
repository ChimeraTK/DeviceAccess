/*
* ScalarAccessor.h
*
*  Created on: Jun 07, 2016
*      Author: Martin Hierholzer
*/

#ifndef CHIMERATK_SCALAR_ACCESSOR_H
#define CHIMERATK_SCALAR_ACCESSOR_H

#include <string>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <mtca4u/ScalarRegisterAccessor.h>

#include "Module.h"
#include "VariableNetworkNode.h"

namespace ChimeraTK {

  /** Accessor for scalar variables (i.e. single values). Note for users: Use the convenience classes
  *  ScalarPollInput, ScalarPushInput, ScalarOutput instead of this class directly. */
  template< typename UserType >
  class ScalarAccessor : public mtca4u::ScalarRegisterAccessor<UserType> {
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
      void replace(const ScalarAccessor<UserType> &newAccessor) {
        mtca4u::NDRegisterAccessorBridge<UserType>::replace(newAccessor);
        node = VariableNetworkNode(this, newAccessor.node.getName(), newAccessor.node.getDirection(), newAccessor.node.getUnit(),
                                  newAccessor.node.getNumberOfElements(), newAccessor.node.getMode(), newAccessor.node.getDescription(),
                                  &newAccessor.node.getValueType());
        if(_owner != newAccessor._owner) {
          if(_owner != nullptr) _owner->unregisterAccessor(*this);
          _owner = newAccessor._owner;
          _owner->registerAccessor(*this);
        }
      }

      void replace(const NDRegisterAccessorBridge<UserType> &newAccessor) = delete;

      using mtca4u::ScalarRegisterAccessor<UserType>::operator=;

      ~ScalarAccessor() {
        if(_owner != nullptr) _owner->unregisterAccessor(*this);
      }
      
  protected:

      ScalarAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode, const std::string &description)
      : node(this, name, direction, unit, 1, mode, description, &typeid(UserType)), _owner(owner)
      {
        owner->registerAccessor(*this);
      }

      /** Default constructor creates a dysfunctional accessor (to be assigned with a real accessor later) */
      ScalarAccessor() {}
    
      VariableNetworkNode node;

      Module *_owner{nullptr};

  };

  /** Convenience class for input scalar accessors with UpdateMode::push */
  template< typename UserType >
  struct ScalarPushInput : public ScalarAccessor<UserType> {
    ScalarPushInput(Module *owner, const std::string &name, std::string unit, const std::string &description)
    : ScalarAccessor<UserType>(owner, name, VariableDirection::consuming, unit, UpdateMode::push, description)
    {}
    ScalarPushInput() : ScalarAccessor<UserType>() {}
    using ScalarAccessor<UserType>::operator=;
  };

  /** Convenience class for input scalar accessors with UpdateMode::poll */
  template< typename UserType >
  struct ScalarPollInput : public ScalarAccessor<UserType> {
    ScalarPollInput(Module *owner, const std::string &name, std::string unit, const std::string &description)
    : ScalarAccessor<UserType>(owner, name, VariableDirection::consuming, unit, UpdateMode::poll, description)
    {}
    ScalarPollInput() : ScalarAccessor<UserType>() {}
    void doReadTransfer() override { this->doReadTransferNonBlocking(); }
    void read() { this->readNonBlocking(); }
    using ScalarAccessor<UserType>::operator=;
  };

  /** Convenience class for output scalar accessors (always UpdateMode::push) */
  template< typename UserType >
  struct ScalarOutput : public ScalarAccessor<UserType> {
    ScalarOutput(Module *owner, const std::string &name, std::string unit, const std::string &description)
    : ScalarAccessor<UserType>(owner, name, VariableDirection::feeding, unit, UpdateMode::push, description)
    {}
    ScalarOutput() : ScalarAccessor<UserType>() {}
    using ScalarAccessor<UserType>::operator=;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_SCALAR_ACCESSOR_H */
