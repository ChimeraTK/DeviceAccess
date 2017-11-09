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

#include "InversionOfControlAccessor.h"
#include "Profiler.h"

namespace ChimeraTK {

  /** Accessor for array variables (i.e. vectors). Note for users: Use the convenience classes
  *  ArrayPollInput, ArrayPushInput, ArrayOutput instead of this class directly. */
  template< typename UserType >
  class ArrayAccessor :  public mtca4u::OneDRegisterAccessor<UserType>,
                         public InversionOfControlAccessor<ArrayAccessor<UserType>> {
    public:

      using InversionOfControlAccessor<ArrayAccessor<UserType>>::operator VariableNetworkNode;
      using InversionOfControlAccessor<ArrayAccessor<UserType>>::operator>>;
      void replace(const mtca4u::NDRegisterAccessorBridge<UserType> &newAccessor) = delete;
      using InversionOfControlAccessor<ArrayAccessor<UserType>>::replace;
      ArrayAccessor<UserType>& operator=(ArrayAccessor<UserType> &other) = delete;
      using mtca4u::OneDRegisterAccessor<UserType>::operator=;
      
      /** Move constructor */
      ArrayAccessor(ArrayAccessor<UserType> &&other) {
        InversionOfControlAccessor<ArrayAccessor<UserType>>::replace(std::move(other));
      }
      
      /** Move assignment */
      ArrayAccessor<UserType>& operator=(ArrayAccessor<UserType> &&other) {
        // Having a move-assignment operator is required to use the move-assignment operator of a module containing
        // an accessor.
        InversionOfControlAccessor<ArrayAccessor<UserType>>::replace(std::move(other));
        return *this;
      };

      void read() {
        Profiler::stopMeasurement();
        mtca4u::OneDRegisterAccessor<UserType>::read();
        Profiler::startMeasurement();
      }

  protected:
    
      friend class InversionOfControlAccessor<ArrayAccessor<UserType>>;

      ArrayAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          size_t nElements, UpdateMode mode, const std::string &description,
          const std::unordered_set<std::string> &tags={})
      : InversionOfControlAccessor<ArrayAccessor<UserType>>(owner, name, direction, unit, nElements, mode,
                                                            description, &typeid(UserType), tags)
      {}
      
      /** Default constructor creates a dysfunctional accessor (to be assigned with a real accessor later) */
      ArrayAccessor() {}

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
