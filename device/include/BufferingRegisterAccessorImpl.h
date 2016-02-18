/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Feb 11 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_IMPL_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_IMPL_H

#include <vector>
#include "TransferElement.h"

namespace mtca4u {

  // forward declarations
  class DeviceBackend;

  template<typename T>
  class BufferingRegisterAccessor;

  /*********************************************************************************************************************/
  /** Base class for implementations of the BufferingRegisterAccessor. The BufferingRegisterAccessor is merely a
   *  proxy to allow having an actual instance rather than just a pointer to this abstract base class.
   *
   *  Functions to access the underlying std::vector<T> are already implemented in this class, since the vector is
   *  already exposed in the interface and this header-only implementation improves the performance.
   */
  template<typename T>
  class BufferingRegisterAccessorImpl: public TransferElement {
    public:

      /** A virtual base class needs a virtual destructor */
      virtual ~BufferingRegisterAccessorImpl() {};

      /** Read the data from the device, convert it and store in buffer. */
      virtual void read() = 0;

      /** Convert data from the buffer and write to device. */
      virtual void write() = 0;

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       */
      virtual T& operator[](unsigned int index) {
        return cookedBuffer[index];
      }

      /** Return number of elements */
      virtual unsigned int getNumberOfElements() {
        return cookedBuffer.size();
      }

      /** Access data with std::vector-like iterators */
      typedef typename std::vector<T>::iterator iterator;
      typedef typename std::vector<T>::const_iterator const_iterator;
      typedef typename std::vector<T>::reverse_iterator reverse_iterator;
      typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
      virtual iterator begin() { return cookedBuffer.begin(); }
      virtual const_iterator cbegin() const { return cookedBuffer.cbegin(); }
      virtual iterator end() { return cookedBuffer.end(); }
      virtual const_iterator cend() const { return cookedBuffer.cend(); }
      virtual reverse_iterator rbegin() { return cookedBuffer.rbegin(); }
      virtual const_reverse_iterator crbegin() const { return cookedBuffer.crbegin(); }
      virtual reverse_iterator rend() { return cookedBuffer.rend(); }
      virtual const_reverse_iterator crend() const { return cookedBuffer.crend(); }

      /* Swap content of (cooked) buffer with std::vector */
      virtual void swap(std::vector<T> &x) {
        cookedBuffer.swap(x);
      }

    protected:

      /// vector of converted data elements
      std::vector<T> cookedBuffer;

      /// the public interface needs access to protected functions of the TransferElement etc.
      friend class BufferingRegisterAccessor<T>;

  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_IMPL_H */
