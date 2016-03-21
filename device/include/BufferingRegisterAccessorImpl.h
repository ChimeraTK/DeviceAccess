/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Feb 11 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_IMPL_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_IMPL_H

#include <vector>
#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "FixedPointConverter.h"

namespace mtca4u {

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

      /** DO NOT USE. FOR BACKWARDS COMPATIBILITY ONLY.
       *
       *  \deprecated This function is for backwards compatibility with the deprecated RegisterAccessor only.
       *  Return the fixed point converter used to convert the raw data from the device to the type T. If no conversion
       *  by the fixed point converter is required, this function will throw an exception. */
      virtual FixedPointConverter getFixedPointConverter() const = 0;

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register. */
      inline T& operator[](unsigned int index) {
        return cookedBuffer[index];
      }

      /** Return number of elements */
      inline unsigned int getNumberOfElements() {
        return cookedBuffer.size();
      }

    protected:

      /// vector of converted data elements
      std::vector<T> cookedBuffer;

      /// the public interface needs access to protected functions of the TransferElement etc.
      friend class BufferingRegisterAccessor<T>;

      /// for compatibility only
      friend class RegisterAccessor;

  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_IMPL_H */
