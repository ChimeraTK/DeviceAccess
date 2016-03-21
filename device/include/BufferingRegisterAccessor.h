/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include "ForwardDeclarations.h"
#include "BufferingRegisterAccessorImpl.h"

namespace mtca4u {

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a variable of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename T>
  class BufferingRegisterAccessor : public TransferElement {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead.
       */
      BufferingRegisterAccessor(boost::shared_ptr< BufferingRegisterAccessorImpl<T> > impl)
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
        return _impl->cookedBuffer[index];
      }

      /** Return number of elements
       */
      inline unsigned int getNumberOfElements() {
        return _impl->cookedBuffer.size();
      }

      /** Implicit type conversion to user type T to access the first element (often the only element).
       *  This covers already a lot of operations like arithmetics and comparison */
      inline operator T&() {
        return _impl->cookedBuffer[0];
      }

      /** Assignment operator, assigns the first element. */
      inline BufferingRegisterAccessor<T>& operator=(T rightHandSide)
      {
        _impl->cookedBuffer[0] = rightHandSide;
        return *this;
      }

      /** Pre-increment operator for the first element. */
      inline BufferingRegisterAccessor<T>& operator++() {
        return operator=( _impl->cookedBuffer[0] + 1 );
      }

      /** Pre-decrement operator for the first element. */
      inline BufferingRegisterAccessor<T>& operator--() {
        return operator=(_impl->cookedBuffer[0] - 1 );
      }

      /** Post-increment operator for the first element. */
      inline T operator++(int) {
        T v = _impl->cookedBuffer[0];
        operator=( v + 1 );
        return v;
      }

      /** Post-decrement operator for the first element. */
      inline T operator--(int) {
        T v = _impl->cookedBuffer[0];
        operator=( v - 1 );
        return v;
      }

      /** Access data with std::vector-like iterators
       */
      typedef typename std::vector<T>::iterator iterator;
      typedef typename std::vector<T>::const_iterator const_iterator;
      typedef typename std::vector<T>::reverse_iterator reverse_iterator;
      typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
      inline iterator begin() { return _impl->cookedBuffer.begin(); }
      inline const_iterator begin() const { return _impl->cookedBuffer.cbegin(); }
      inline const_iterator cbegin() const { return _impl->cookedBuffer.cbegin(); }
      inline iterator end() { return _impl->cookedBuffer.end(); }
      inline const_iterator end() const { return _impl->cookedBuffer.cend(); }
      inline const_iterator cend() const { return _impl->cookedBuffer.cend(); }
      inline reverse_iterator rbegin() { return _impl->cookedBuffer.rbegin(); }
      inline const_reverse_iterator rbegin() const { return _impl->cookedBuffer.crbegin(); }
      inline const_reverse_iterator crbegin() const { return _impl->cookedBuffer.crbegin(); }
      inline reverse_iterator rend() { return _impl->cookedBuffer.rend(); }
      inline const_reverse_iterator rend() const { return _impl->cookedBuffer.crend(); }
      inline const_reverse_iterator crend() const { return _impl->cookedBuffer.crend(); }

      /* Swap content of (cooked) buffer with std::vector
       */
      inline void swap(std::vector<T> &x) {
        _impl->cookedBuffer.swap(x);
      }

      virtual bool isReadOnly() const {
        return _impl->isReadOnly();
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        return _impl->isSameRegister(other);
      }

      virtual bool isSameRegister(const TransferElement &other) const {
        return other.isSameRegister(_impl);
      }

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _impl->getHardwareAccessingElements();
      }

      virtual boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return boost::static_pointer_cast<TransferElement>(_impl);
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_impl->isSameRegister(newElement)) {
          _impl = boost::dynamic_pointer_cast< BufferingRegisterAccessorImpl<T> >(newElement);
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
      }

      /** DEPRECATED
       *
       *  \deprecated This function is deprecated. Just pass around copies of the BufferingRegisterAccessor itself
       *  instead of shared pointers, which will create the exact same behaviour. */
      boost::shared_ptr< BufferingRegisterAccessorImpl<T> > getSharedPtr() {
        std::cerr << "##################################################################################" << std::endl;
        std::cerr << "# The function BufferingRegisterAccessor::getSharedPtr() is depcreated." << std::endl;
        std::cerr << "# Just pass around copies of the TwoDRegisterAccessor itself instead of shared" << std::endl;
        std::cerr << "# pointers, which will create the exact same behaviour." << std::endl;
        std::cerr << "##################################################################################" << std::endl;
        return _impl;
      }

    protected:

      /// pointer to the implementation
      boost::shared_ptr< BufferingRegisterAccessorImpl<T> > _impl;

      // the TransferGroup must be a friend to access the actual accesor
      friend class TransferGroup;

  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
