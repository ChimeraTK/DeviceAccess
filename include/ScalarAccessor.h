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

#include "Accessor.h"

namespace ChimeraTK {

  /** Accessor for scalar variables (i.e. single values). Note for users: Preferrably use the convenience classes
   *  ScalarPollInput, ScalarPushInput, ScalarOutput instead of this class directly. */
  template< typename UserType >
  class ScalarAccessor : public Accessor<UserType> {
    public:
      ScalarAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode, const std::string &description)
      : Accessor<UserType>(owner, name, direction, unit, 1, mode, description)
      {}

      /** Default constructor creates a dysfunction accessor (to be assigned with a real accessor later) */
      ScalarAccessor() {}

      void read() {
        if(Accessor<UserType>::_mode == UpdateMode::push) {
          while(impl->readNonBlocking() == false) { /// @todo TODO proper blocking implementation
            boost::this_thread::yield();
            boost::this_thread::interruption_point();
          }
        }
        else {
          /// @todo TODO empty the queue to always receive the latest value
          impl->readNonBlocking();
          boost::this_thread::interruption_point();
        }
      } // LCOV_EXCL_LINE this line somehow ends up having a negative counter in the coverage report, which leads to a failure

      void write() {
        impl->write();
        boost::this_thread::interruption_point();
      }
      
      bool readNonBlocking() {
        boost::this_thread::interruption_point();
        return impl->readNonBlocking();
      }
        
      /** Implicit type conversion to user type T to access the first element (often the only element).
       *  This covers already a lot of operations like arithmetics and comparison */
      operator UserType() {
        return impl->accessData(0);
      }

      /** Assignment operator */
      ScalarAccessor<UserType>& operator=(UserType rightHandSide) {
        impl->accessData(0) = rightHandSide;
        return *this;
      }

      /** Pre-increment operator */
      ScalarAccessor<UserType>& operator++() {
        impl->accessData(0) = ++(impl->accessData(0));
        return *this;
      }

      /** Pre-decrement operator */
      ScalarAccessor<UserType>& operator--() {
        impl->accessData(0) = --(impl->accessData(0));
        return *this;
      }

      /** Post-increment operator */
      UserType operator++(int) {
        UserType temp = impl->accessData(0);
        impl->accessData(0) = temp+1;
        return temp;
      }

      /** Post-decrement operator */
      UserType operator--(int) {
        UserType temp = impl->accessData(0);
        impl->accessData(0) = temp-1;
        return temp;
      }

      bool isInitialised() const {
        return impl != nullptr;
      }

      void useProcessVariable(const boost::shared_ptr<TransferElement > &var) {
        impl = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(var);
        assert(impl);
        if(Accessor<UserType>::getDirection() == VariableDirection::consuming) {
          assert(impl->isReadable());
        }
        else {
          assert(impl->isWriteable());
        }
      }
      
      using Accessor<UserType>::impl;

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
