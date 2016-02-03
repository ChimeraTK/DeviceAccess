/*
 * RegisterAccessor2D.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef REGISTERACCESSOR2D_H_
#define REGISTERACCESSOR2D_H_

#include <boost/smart_ptr.hpp>

namespace mtca4u {

  template<class UserType>
  class MultiplexedDataAccessor;

  template<class UserType>
  class RegisterAccessor2D {

    public:

      /** Do not use this constructor directly. Instead call Device::getRegisterAccessor2D().
       */
      RegisterAccessor2D( boost::shared_ptr< MultiplexedDataAccessor<UserType> > _accessor )
      : accessor(_accessor)
      {}

      /** Operator to access individual sequences.
       */
      std::vector<UserType> & operator[](size_t sequenceIndex) {
        return accessor->operator[](sequenceIndex);
      }

      /** Read the data from the device, de-multiplex the hardware IO buffer and
       *  fill the sequence buffers using the fixed point converters. The read
       *  method will handle reads into the DMA regions as well
       */
      void read() {
        accessor->read();
      }

      /** Multiplex the data from the sequence buffer into the hardware IO buffer,
       * using the fixed point converters, and write it to the device. Can be used
       * to write to DMA memory Areas, but this functionality has not been
       * implemented yet
       */
      void write() {
        accessor->write();
      }

      /**
       * Return the number of sequences that have been Multiplexed
       */
      size_t getNumberOfDataSequences() {
        return accessor->getNumberOfDataSequences();
      }

      /**
       * Default destructor
       */
      ~RegisterAccessor2D() {
      }

    protected:

      boost::shared_ptr< MultiplexedDataAccessor<UserType> > accessor;

  };

} // namespace mtca4u

#endif /* REGISTERACCESSOR2D_H_ */
