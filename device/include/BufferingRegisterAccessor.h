/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorBridge.h"
#include "DeviceException.h"

namespace mtca4u {

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a variable of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename UserType>
  class BufferingRegisterAccessor : public  NDRegisterAccessorBridge<UserType> {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead. */
      BufferingRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : NDRegisterAccessorBridge<UserType>(impl)
      {
        if(NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels() != 1) {
          throw DeviceException("The BufferingRegisterAccessor has a too low dimension to access this register.",
              DeviceException::WRONG_ACCESSOR);
        }
      }

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      BufferingRegisterAccessor() {}


      /** Read the data from the device, convert it and store in buffer. */
      void read() {
        NDRegisterAccessorBridge<UserType>::_impl->read();
      }

      /** Convert data from the buffer and write to device. */
      void write() {
        NDRegisterAccessorBridge<UserType>::_impl->write();
      }

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       */
      UserType& operator[](unsigned int index) {
        return NDRegisterAccessorBridge<UserType>::_impl->accessData(0,index);
      }

      /** Return number of elements
       */
      unsigned int getNumberOfElements() {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
      }

      /** Implicit type conversion to user type T to access the first element (often the only element).
       *  This covers already a lot of operations like arithmetics and comparison */
      operator UserType&() {
        return NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
      }

      /** Assignment operator, assigns the first element. */
      BufferingRegisterAccessor<UserType>& operator=(UserType rightHandSide)
      {
        NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) = rightHandSide;
        return *this;
      }

      /** Pre-increment operator for the first element. */
      BufferingRegisterAccessor<UserType>& operator++() {
        return operator=( NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) + 1 );
      }

      /** Pre-decrement operator for the first element. */
      BufferingRegisterAccessor<UserType>& operator--() {
        return operator=( NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) - 1 );
      }

      /** Post-increment operator for the first element. */
      UserType operator++(int) {
        UserType v = NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
        operator=( v + 1 );
        return v;
      }

      /** Post-decrement operator for the first element. */
      UserType operator--(int) {
        UserType v = NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
        operator=( v - 1 );
        return v;
      }

      /** Access data with std::vector-like iterators */
      typedef typename std::vector<UserType>::iterator iterator;
      typedef typename std::vector<UserType>::const_iterator const_iterator;
      typedef typename std::vector<UserType>::reverse_iterator reverse_iterator;
      typedef typename std::vector<UserType>::const_reverse_iterator const_reverse_iterator;
      iterator begin() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).begin(); }
      const_iterator begin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cbegin(); }
      const_iterator cbegin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cbegin(); }
      iterator end() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).end(); }
      const_iterator end() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cend(); }
      const_iterator cend() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cend(); }
      reverse_iterator rbegin() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).rbegin(); }
      const_reverse_iterator rbegin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crbegin(); }
      const_reverse_iterator crbegin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crbegin(); }
      reverse_iterator rend() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).rend(); }
      const_reverse_iterator rend() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crend(); }
      const_reverse_iterator crend() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crend(); }

      /* Swap content of (cooked) buffer with std::vector */
      void swap(std::vector<UserType> &x) {
        NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).swap(x);
      }

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadOnly();
      }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return NDRegisterAccessorBridge<UserType>::_impl != NULL;
      }

      friend class TransferGroup;
  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
