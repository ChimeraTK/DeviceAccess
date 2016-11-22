/*
 * ScalarRegisterAccessor.h
 *
 *  Created on: Mar 23, 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_SCALAR_REGISTER_ACCESSOR_H
#define MTCA4U_SCALAR_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorBridge.h"
#include "DeviceException.h"

namespace mtca4u {

  /*********************************************************************************************************************/
  /** Accessor class to read and write scalar registers transparently by using the accessor object like a variable of
   *  the type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map, if required. Obtain the accessor using the Device::getScalarRegisterAccessor()
   *  function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename UserType>
  class ScalarRegisterAccessor : public NDRegisterAccessorBridge<UserType> {
    public:

      /** Constructor. @attention Do not normally use directly.
       *  Users should call Device::getScalarRegisterAccessor() to obtain an instance instead. */
      ScalarRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : NDRegisterAccessorBridge<UserType>(impl)
      {}

      /** Placeholder constructor, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      ScalarRegisterAccessor() {}

      /** Read the data from the device, convert it and store in buffer. */
      void read() {
        NDRegisterAccessorBridge<UserType>::_impl->read();
      }

      bool readNonBlocking(){
        return NDRegisterAccessorBridge<UserType>::_impl->readNonBlocking();
      }

      /** Convert data from the buffer and write to device. */
      void write() {
        NDRegisterAccessorBridge<UserType>::_impl->write();
      }

      /** Implicit type conversion to user type T to access the first element (often the only element).
       *  This covers already a lot of operations like arithmetics and comparison */
      operator UserType&() {
        return NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
      }

      /** Assignment operator, assigns the first element. */
      ScalarRegisterAccessor<UserType>& operator=(UserType rightHandSide)
      {
        NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) = rightHandSide;
        return *this;
      }

      /** Pre-increment operator for the first element. */
      ScalarRegisterAccessor<UserType>& operator++() {
        return operator=( NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) + 1 );
      }

      /** Pre-decrement operator for the first element. */
      ScalarRegisterAccessor<UserType>& operator--() {
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

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadOnly();
      }

      bool isReadable() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadable();
      }

      bool isWriteable() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isWriteable();
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

#endif /* MTCA4U_SCALAR_REGISTER_ACCESSOR_H */
