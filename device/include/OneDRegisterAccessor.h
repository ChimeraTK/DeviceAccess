/*
 * OneDRegisterAccessor.h
 *
 *  Created on: Mar 23, 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_ONE_D_REGISTER_ACCESSOR_H
#define MTCA4U_ONE_D_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorBridge.h"
#include "DeviceException.h"

namespace mtca4u {

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a vector of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename UserType>
  class OneDRegisterAccessor : public NDRegisterAccessorBridge<UserType> {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getOneDRegisterAccessor() to obtain an instance instead. */
      OneDRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : NDRegisterAccessorBridge<UserType>(impl)
      {
        if(NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels() != 1) {
          throw DeviceException("The OneDRegisterAccessor has a too low dimension to access this register.",
              DeviceException::WRONG_ACCESSOR);
        }
      }

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      OneDRegisterAccessor() {}

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       *  Note: Using the iterators is slightly more efficient than using this operator! */
      UserType& operator[](unsigned int element) {
        return NDRegisterAccessorBridge<UserType>::_impl->accessData(0,element);
      }

      /** Return number of elements/samples in the register */
      unsigned int getNElements() {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
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
        if(x.size() != NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).size()) {
          throw DeviceException("Swapping with a buffer of a different size is not allowed.",
              DeviceException::WRONG_PARAMETER);
        }
        NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).swap(x);
      }

      /** Return a direct pointer to the memory buffer storng the elements.
       *  @attention Note that this pointer will be invalidated during read(), write() and swap(). If this accessor is
       *  part of a TransferGroup, any call to one of these functions on any element of the TransferGroup or the
       *  TransferGroup itself may invalidate the pointer! */
      UserType* data() {
        return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).data();
      }

      friend class TransferGroup;
  };

}    // namespace mtca4u

#endif /* MTCA4U_ONE_D_REGISTER_ACCESSOR_H */
