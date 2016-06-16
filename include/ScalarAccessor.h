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

#include "Accessor.h"

/** Macro to declare a scalar variable/accessor more easily. The call to this macro must be placed inside the
 *  class definiton of an ApplicationModule. */
#define SCALAR_ACCESSOR(UserType, name, direction, unit, mode)                                                      \
    ChimeraTK::ScalarAccessor<UserType> name{this, #name, direction, unit, mode}

namespace ChimeraTK {

  /** Accessor for scalar variables (i.e. single values). */
  template< typename UserType >
  class ScalarAccessor : public Accessor<UserType> {
    public:
      ScalarAccessor(ApplicationModule *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode)
      : Accessor<UserType>(owner, name, direction, unit, mode)
      {}

      /** Read an input variable. In case of an output variable, an exception will be thrown. This function will block
       *  the calling thread until the variable has been read. If the UpdateMode::push flag has been set when creating
       *  the accessor, this function will wait until a new value has been provided to the variable. If a new value is
       *  already available before calling this function, the function will be non-blocking and lock-free. */
      void read() {
        if(Accessor<UserType>::_mode == UpdateMode::push) {
          while(impl->receive() == false) usleep(1); // @todo TODO proper blocking implementation
        }
        else {
          while(impl->receive() == true);
        }
      }

      /** Check if an input variable has new data. In case of an output variable, an exception will be thrown. If the
       *  wait_for_new_data access mode flag was not provided when creating the accessor, this function will return
       *  always false. */
      bool hasNewData();

      /** Write an output variable. In case of an input variable, an exception will be thrown. This function never
       *  blocks and is always implemented in a lock-free manner. */
      void write() {
        impl->send();
      }

      /** Implicit type conversion to user type T to access the first element (often the only element).
       *  This covers already a lot of operations like arithmetics and comparison */
      operator UserType() {
        return impl->get();
      }

      /** Assignment operator */
      ScalarAccessor<UserType>& operator=(UserType rightHandSide) {
        impl->set(rightHandSide);
        return *this;
      }

      /** Pre-increment operator */
      ScalarAccessor<UserType>& operator++() {
        impl->set(++(impl->get()));
        return *this;
      }

      /** Pre-decrement operator */
      ScalarAccessor<UserType>& operator--() {
        impl->set(--(impl->get()));
        return *this;
      }

      /** Post-increment operator */
      UserType operator++(int) {
        UserType temp = impl->get();
        impl->set(temp+1);
        return temp;
      }

      /** Post-decrement operator */
      UserType operator--(int) {
        UserType temp = impl->get();
        impl->set(temp-1);
        return temp;
      }

      bool isInitialised() const {
        return impl != nullptr;
      }

      void useProcessVariable(const boost::shared_ptr<ProcessVariable> &var) {
        impl = boost::dynamic_pointer_cast< ProcessScalar<UserType> >(var);
        if(!impl) {
          throw std::string("ProcessVariable of the wrong type provided, cannot be used as the implementation!"); // @todo TODO throw proper exception
        }
      }

    protected:

      boost::shared_ptr< ProcessScalar<UserType> > impl;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_SCALAR_ACCESSOR_H */
