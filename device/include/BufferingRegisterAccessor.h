/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include "BufferingRegisterAccessorImpl.h"

namespace mtca4u {

  // forward declaration to make a friend
  class TransferGroup;

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a variable of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename T>
  class BufferingRegisterAccessor : protected TransferElement {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead.
       */
      BufferingRegisterAccessor(boost::shared_ptr<BufferingRegisterAccessorImpl<T>> impl)
      : _impl(impl)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional!
       */
      BufferingRegisterAccessor() {}

      /** destructor
       */
      ~BufferingRegisterAccessor() {};


      /** Read the data from the device, convert it and store in buffer.
       */
      inline void read() {
       _impl->read();
      }

      /** Convert data from the buffer and write to device.
       */
      inline void write() {
        _impl->write();
      }

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       */
      inline T& operator[](unsigned int index) {
        return _impl->operator[](index);
      }

      /** Return number of elements
       */
      inline unsigned int getNumberOfElements() {
        return _impl->getNumberOfElements();
      }

      /** Access data with std::vector-like iterators
       */
      typedef typename std::vector<T>::iterator iterator;
      typedef typename std::vector<T>::const_iterator const_iterator;
      typedef typename std::vector<T>::reverse_iterator reverse_iterator;
      typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
      inline iterator begin() { return _impl->begin(); }
      inline const_iterator begin() const { return _impl->begin(); }
      inline iterator end() { return _impl->end(); }
      inline const_iterator end() const { return _impl->end(); }
      inline reverse_iterator rbegin() { return _impl->rbegin(); }
      inline const_reverse_iterator rbegin() const { return _impl->rbegin(); }
      inline reverse_iterator rend() { return _impl->rend(); }
      inline const_reverse_iterator rend() const { return _impl->rend(); }

      /* Swap content of (cooked) buffer with std::vector
       */
      inline void swap(std::vector<T> &x) {
        _impl->swap(x);
      }

    protected:

      /// pointer to the implementation
      boost::shared_ptr< BufferingRegisterAccessorImpl<T> > _impl;

      // the TransferGroup must be a friend to access the actual accesor
      friend class TransferGroup;

    public:

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        return _impl->isSameRegister(other);
      }

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _impl->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_impl->isSameRegister(newElement)) {
          _impl = boost::dynamic_pointer_cast< BufferingRegisterAccessorImpl<T> >(newElement);
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
      }

  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
