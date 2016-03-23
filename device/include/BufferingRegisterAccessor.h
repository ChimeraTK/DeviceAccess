/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"

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
  class BufferingRegisterAccessor {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead. */
      BufferingRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<T> > impl)
      : _impl(impl)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      BufferingRegisterAccessor() {}


      /** Read the data from the device, convert it and store in buffer. */
      void read() {
       _impl->read();
      }

      /** Convert data from the buffer and write to device. */
      void write() {
        _impl->write();
      }

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       */
      T& operator[](unsigned int index) {
        return _impl->buffer_2D[0][index];
      }

      /** Return number of elements
       */
      unsigned int getNumberOfElements() {
        return _impl->buffer_2D[0].size();
      }

      /** Implicit type conversion to user type T to access the first element (often the only element).
       *  This covers already a lot of operations like arithmetics and comparison */
      operator T&() {
        return _impl->buffer_2D[0][0];
      }

      /** Assignment operator, assigns the first element. */
      BufferingRegisterAccessor<T>& operator=(T rightHandSide)
      {
        _impl->buffer_2D[0][0] = rightHandSide;
        return *this;
      }

      /** Pre-increment operator for the first element. */
      BufferingRegisterAccessor<T>& operator++() {
        return operator=( _impl->buffer_2D[0][0] + 1 );
      }

      /** Pre-decrement operator for the first element. */
      BufferingRegisterAccessor<T>& operator--() {
        return operator=(_impl->buffer_2D[0][0] - 1 );
      }

      /** Post-increment operator for the first element. */
      T operator++(int) {
        T v = _impl->buffer_2D[0][0];
        operator=( v + 1 );
        return v;
      }

      /** Post-decrement operator for the first element. */
      T operator--(int) {
        T v = _impl->buffer_2D[0][0];
        operator=( v - 1 );
        return v;
      }

      /** Access data with std::vector-like iterators
       */
      typedef typename std::vector<T>::iterator iterator;
      typedef typename std::vector<T>::const_iterator const_iterator;
      typedef typename std::vector<T>::reverse_iterator reverse_iterator;
      typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
      iterator begin() { return _impl->buffer_2D[0].begin(); }
      const_iterator begin() const { return _impl->buffer_2D[0].cbegin(); }
      const_iterator cbegin() const { return _impl->buffer_2D[0].cbegin(); }
      iterator end() { return _impl->buffer_2D[0].end(); }
      const_iterator end() const { return _impl->buffer_2D[0].cend(); }
      const_iterator cend() const { return _impl->buffer_2D[0].cend(); }
      reverse_iterator rbegin() { return _impl->buffer_2D[0].rbegin(); }
      const_reverse_iterator rbegin() const { return _impl->buffer_2D[0].crbegin(); }
      const_reverse_iterator crbegin() const { return _impl->buffer_2D[0].crbegin(); }
      reverse_iterator rend() { return _impl->buffer_2D[0].rend(); }
      const_reverse_iterator rend() const { return _impl->buffer_2D[0].crend(); }
      const_reverse_iterator crend() const { return _impl->buffer_2D[0].crend(); }

      /* Swap content of (cooked) buffer with std::vector */
      void swap(std::vector<T> &x) {
        _impl->buffer_2D[0].swap(x);
      }

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const {
        return _impl->isReadOnly();
      }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return _impl != NULL;
      }

    protected:

      /** pointer to the implementation */
      boost::shared_ptr< NDRegisterAccessor<T> > _impl;

      /** return the implementation, used for adding the accessor to a TransferGroup */
      boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return boost::static_pointer_cast<TransferElement>(_impl);
      }

      friend class TransferGroup;
  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
