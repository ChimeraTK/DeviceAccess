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
      
      /** Change meta data (name, unit, description and optionally tags). This function may only be used on
       *  Application-type nodes. If the optional argument tags is omitted, the tags will not be changed. To clear the
       *  tags, an empty set can be passed. */
      void setMetaData(const std::string &name, const std::string &unit, const std::string &description) {
        node.setMetaData(name,unit,description);
      }
      void setMetaData(const std::string &name, const std::string &unit, const std::string &description,
                       const std::unordered_set<std::string> &tags) {
        node.setMetaData(name,unit,description,tags);
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

      void replace(const mtca4u::NDRegisterAccessorBridge<UserType> &newAccessor) = delete;

      using mtca4u::ScalarRegisterAccessor<UserType>::operator=;

      /** Move-assignment operator as an alternative for replace where applicable. This is needed to allow late
       *  initialisation of ApplicationModules using ScalarAccessors */
      ScalarAccessor<UserType>& operator=(ScalarAccessor<UserType> &&rhs) {
        mtca4u::NDRegisterAccessorBridge<UserType>::replace(rhs);
        node.pdata = rhs.node.pdata;
        node.pdata->appNode = this;
        return *this;
      }
      
      ~ScalarAccessor() {
        if(_owner != nullptr) _owner->unregisterAccessor(*this);
      }
      
      /** Add a tag. Valid names for tags only contain alpha-numeric characters (i.e. no spaces and no special
       *  characters). */
      void addTag(const std::string &tag) {
        node.addTag(tag);
      }
      
  protected:

      ScalarAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode, const std::string &description, const std::unordered_set<std::string> &tags={})
      : node(this, name, direction, unit, 1, mode, description, &typeid(UserType), tags), _owner(owner)
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
    ScalarPushInput(Module *owner, const std::string &name, std::string unit,
                    const std::string &description, const std::unordered_set<std::string> &tags={})
    : ScalarAccessor<UserType>(owner, name, VariableDirection::consuming, unit, UpdateMode::push,
                               description, tags)
    {}
    ScalarPushInput() : ScalarAccessor<UserType>() {}
    using ScalarAccessor<UserType>::operator=;
  };

  /** Convenience class for input scalar accessors with UpdateMode::poll */
  template< typename UserType >
  struct ScalarPollInput : public ScalarAccessor<UserType> {
    ScalarPollInput(Module *owner, const std::string &name, std::string unit,
                    const std::string &description, const std::unordered_set<std::string> &tags={})
    : ScalarAccessor<UserType>(owner, name, VariableDirection::consuming, unit, UpdateMode::poll,
                               description, tags)
    {}
    ScalarPollInput() : ScalarAccessor<UserType>() {}
    void doReadTransfer() override { this->doReadTransferLatest(); }
    void read() { this->readLatest(); }
    using ScalarAccessor<UserType>::operator=;
  };

  /** Convenience class for output scalar accessors (always UpdateMode::push) */
  template< typename UserType >
  struct ScalarOutput : public ScalarAccessor<UserType> {
    ScalarOutput(Module *owner, const std::string &name, std::string unit,
                 const std::string &description, const std::unordered_set<std::string> &tags={})
    : ScalarAccessor<UserType>(owner, name, VariableDirection::feeding, unit, UpdateMode::push,
                               description, tags)
    {}
    ScalarOutput() : ScalarAccessor<UserType>() {}
    using ScalarAccessor<UserType>::operator=;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_SCALAR_ACCESSOR_H */
