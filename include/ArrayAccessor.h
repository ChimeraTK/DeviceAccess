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

#include "Accessor.h"

namespace ChimeraTK {

  /** Accessor for array variables (i.e. vectors). Note for users: Preferrably use the convenience classes
   *  ArrayPollInput, ArrayPushInput, ArrayOutput instead of this class directly. */
  template< typename UserType >
  class ArrayAccessor : public Accessor<UserType> {
    public:
      ArrayAccessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit,
          size_t nElements, UpdateMode mode, const std::string &description)
      : Accessor<UserType>(owner, name, direction, unit, nElements, mode, description)
      {}

      /** Default constructor creates a dysfunction accessor (to be assigned with a real accessor later) */
      ArrayAccessor() {}

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

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       *  Note: Using the iterators is slightly more efficient than using this operator! */
      UserType& operator[](unsigned int element) {
        return impl->accessData(0,element);
      }

      /** Return number of elements/samples in the register */
      unsigned int getNElements() {
        return impl->getNumberOfSamples();
      }

      /** Access data with std::vector-like iterators */
      typedef typename std::vector<UserType>::iterator iterator;
      typedef typename std::vector<UserType>::const_iterator const_iterator;
      typedef typename std::vector<UserType>::reverse_iterator reverse_iterator;
      typedef typename std::vector<UserType>::const_reverse_iterator const_reverse_iterator;
      iterator begin() { return impl->accessChannel(0).begin(); }
      const_iterator begin() const { return impl->accessChannel(0).cbegin(); }
      const_iterator cbegin() const { return impl->accessChannel(0).cbegin(); }
      iterator end() { return impl->accessChannel(0).end(); }
      const_iterator end() const { return impl->accessChannel(0).cend(); }
      const_iterator cend() const { return impl->accessChannel(0).cend(); }
      reverse_iterator rbegin() { return impl->accessChannel(0).rbegin(); }
      const_reverse_iterator rbegin() const { return impl->accessChannel(0).crbegin(); }
      const_reverse_iterator crbegin() const { return impl->accessChannel(0).crbegin(); }
      reverse_iterator rend() { return impl->accessChannel(0).rend(); }
      const_reverse_iterator rend() const { return impl->accessChannel(0).crend(); }
      const_reverse_iterator crend() const { return impl->accessChannel(0).crend(); }

      /* Swap content of (cooked) buffer with std::vector */
      void swap(std::vector<UserType> &x) {
        if(x.size() != impl->accessChannel(0).size()) {
          throw DeviceException("Swapping with a buffer of a different size is not allowed.",
              DeviceException::WRONG_PARAMETER);
        }
        impl->accessChannel(0).swap(x);
      }        

      /** Assignment operator */
      ArrayAccessor<UserType>& operator=(const std::vector<UserType> &rightHandSide) {
        impl->accessChannel(0) = rightHandSide;
        return *this;
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
