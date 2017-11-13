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

#include "InversionOfControlAccessor.h"
#include "Profiler.h"

namespace ChimeraTK {

  /** Accessor for scalar variables (i.e. single values). Note for users: Use the convenience classes
  *  ScalarPollInput, ScalarPushInput, ScalarOutput instead of this class directly. */
  template< typename UserType >
  class ScalarAccessor : public mtca4u::ScalarRegisterAccessor<UserType>,
                         public InversionOfControlAccessor<ScalarAccessor<UserType>> {
    public:

      using InversionOfControlAccessor<ScalarAccessor<UserType>>::operator VariableNetworkNode;
      using InversionOfControlAccessor<ScalarAccessor<UserType>>::operator>>;
      void replace(const mtca4u::NDRegisterAccessorBridge<UserType> &newAccessor) = delete;
      using InversionOfControlAccessor<ScalarAccessor<UserType>>::replace;
      ScalarAccessor<UserType>& operator=(ScalarAccessor<UserType> &other) = delete;
      using mtca4u::ScalarRegisterAccessor<UserType>::operator=;
      
      /** Move constructor */
      ScalarAccessor(ScalarAccessor<UserType> &&other) {
        InversionOfControlAccessor<ScalarAccessor<UserType>>::replace(std::move(other));
      }

      /** Move assignment. */
      ScalarAccessor<UserType>& operator=(ScalarAccessor<UserType> &&other) {
        // Having a move-assignment operator is required to use the move-assignment operator of a module containing
        // an accessor.
        InversionOfControlAccessor<ScalarAccessor<UserType>>::replace(std::move(other));
        return *this;
      }

      void read() override {
        Profiler::stopMeasurement();
        mtca4u::ScalarRegisterAccessor<UserType>::read();
        Profiler::startMeasurement();
      }

  protected:
    
      friend class InversionOfControlAccessor<ScalarAccessor<UserType>>;

      ScalarAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode, const std::string &description,
          const std::unordered_set<std::string> &tags={})
      : InversionOfControlAccessor<ScalarAccessor<UserType>>(owner, name, direction, unit, 1, mode,
                                                            description, &typeid(UserType), tags)
      {}
      
      /** Default constructor creates a dysfunctional accessor (to be assigned with a real accessor later) */
      ScalarAccessor() {}

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
    void read() override { this->readLatest(); }
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
