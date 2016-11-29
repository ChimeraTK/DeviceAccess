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

/** Macros to declare a scalar variable/accessor more easily. The call to this macro must be placed inside the
 *  class definiton of a Module (e.g. ApplicationModule or VariableGroup).
 *
 *  UserType is the data type of the variable.
 *  name will be the C++ symbol name of the variable accessor. It will be of the type ChimeraTK::ScalarAccessor<UserType>
 *  unit is the engineering unit as a character constant.
 *  mode can be either ChimeraTK::UpdateMode::push or ChimeraTK::UpdateMode::poll, deciding whether a call to read()
 *  will block until new data is available (push) or just return the latest value (poll, might not be fully realtime
 *  capable). */
#define CTK_SCALAR_INPUT(UserType, name, unit, mode, description)                                                   \
    ChimeraTK::ScalarAccessor<UserType> name{this, #name, ChimeraTK::VariableDirection::consuming, unit, mode,      \
                                             description}
#define CTK_SCALAR_OUTPUT(UserType, name, unit, description)                                                        \
    ChimeraTK::ScalarAccessor<UserType> name{this, #name, ChimeraTK::VariableDirection::feeding, unit,              \
                                             ChimeraTK::UpdateMode::push, description}

namespace ChimeraTK {

  /** Accessor for scalar variables (i.e. single values). */
  template< typename UserType >
  class ScalarAccessor : public Accessor<UserType> {
    public:
      ScalarAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode, const std::string &description)
      : Accessor<UserType>(owner, name, direction, unit, 1, mode, description)
      {}

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

    protected:

      boost::shared_ptr< NDRegisterAccessor<UserType> > impl;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_SCALAR_ACCESSOR_H */
