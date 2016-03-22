/*
 * NDRegisterAccessor.h - N-dimensional register acceesor
 *
 *  Created on: Mar 21, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_N_D_REGISTER_ACCESSOR_H
#define MTCA4U_N_D_REGISTER_ACCESSOR_H

#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  /** N-dimensional register accessor. Base class for all register accessor implementations. The user frontend classes
   *  BufferingRegisterAccessor and TwoDRegisterAccessor are using implementations based on this class to perform
   *  the actual IO. */
  template<typename UserType>
  class NDRegisterAccessor : public TransferElement {

    public:

      /** A virtual base class needs a virtual destructor */
      virtual ~NDRegisterAccessor() {};

      /** Read the data from the device, convert it and store in buffer. */
      virtual void read() = 0;

      /** Convert data from the buffer and write to device. */
      virtual void write() = 0;

      /** Get or set register accessor's buffer content (1D version).
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register. */
      UserType& accessData(unsigned int index) {
        return buffer_2D[0][index];
      }

      /** Get or set register accessor's buffer content (2D version).
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register. */
      UserType& accessData(unsigned int channel, unsigned int index) {
        return buffer_2D[channel][index];
      }

      /** Return number of elements per channel */
      unsigned int getNumberOfSamples() const {
        return buffer_2D[0].size();
      }

      /** Return number of channels */
      unsigned int getNumberOfChannels() const {
        return buffer_2D.size();
      }

      /** DO NOT USE. FOR BACKWARDS COMPATIBILITY ONLY.
       *
       *  \deprecated This function is for backwards compatibility with the deprecated RegisterAccessor only.
       *  Return the fixed point converter used to convert the raw data from the device to the type T. If no conversion
       *  by the fixed point converter is required, this function will throw an exception. */
      virtual FixedPointConverter getFixedPointConverter() const = 0;

    protected:

      /** Buffer of converted data elements. The buffer is always two dimensional. If a register with a single
       *  dimension should be accessed, the outer vector has only a single element. For a scalar register, only a
       *  single element is present in total (buffer_2D[0][0]). This has a negligible performance impact when
       *  optimisations are enabled, but allows a coherent interface for all accessors independent of their dimension.
       *
       *  Implementation note: The buffer must be created with the right number of elements in the constructor! */
      std::vector< std::vector<UserType> > buffer_2D;

      /// the public interfaces needs access to protected functions of the TransferElement etc.
      friend class BufferingRegisterAccessor<UserType>;
      friend class TwoDRegisterAccessor<UserType>;
      friend class MultiplexedDataAccessor<UserType>;
      friend class RegisterAccessor;

  };

}

#endif /* MTCA4U_N_D_REGISTER_ACCESSOR_H */
