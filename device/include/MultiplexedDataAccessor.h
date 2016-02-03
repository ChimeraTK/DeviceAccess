#ifndef _MTCA4U_SEQUENCE_DE_MULTIPLEXER_H_
#define _MTCA4U_SEQUENCE_DE_MULTIPLEXER_H_

#include "RegisterInfoMap.h"
#include "FixedPointConverter.h"
#include "DeviceBackend.h"
#include "MapException.h"
#include "NotImplementedException.h"
#include <sstream>
#include <boost/shared_ptr.hpp>

namespace mtca4u{

  /** Exception class for MultiplexedDataAccessor
   */
  class MultiplexedDataAccessorException : public Exception {
    public:

      enum { EMPTY_AREA, INVALID_WORD_SIZE, INVALID_N_ELEMENTS };


      MultiplexedDataAccessorException(const std::string &message, unsigned int ID)
      : Exception(message, ID){}
  };

  /** Base class which does not depend on the SequenceWordType.
   */
  template<class UserType>
  class MultiplexedDataAccessor{
    public:
      /** Constructor to intialise the members.
       */
      MultiplexedDataAccessor( boost::shared_ptr< DeviceBackend > const & ioDevice );

      /** Operator to access individual sequences.
       */
      std::vector<UserType> & operator[](size_t sequenceIndex);

      /** Read the data from the device, de-multiplex the hardware IO buffer and
       *  fill the sequence buffers using the fixed point converters. The read
       *  method will handle reads into the DMA regions as well
       */
      virtual void read() = 0;

      /** Multiplex the data from the sequence buffer into the hardware IO buffer,
       * using the fixed point converters, and write it to the device. Can be used
       * to write to DMA memory Areas, but this functionality has not been
       * implemented yet
       */
      virtual void write() = 0;

      /**
       * Return the number of sequences that have been Multiplexed
       */
      virtual size_t getNumberOfDataSequences() = 0;

      /**
       * Default destructor
       */
      virtual ~MultiplexedDataAccessor(){};

      /** \deprecated
       * Do not use, only for backwards compatibility.
       * Redefine the user type, as we need it for the backwards-compatible factory Device::getCustomAccessor()
       */
      typedef UserType userType;

    protected:

      /** The converted data for the user space. */
      std::vector< std::vector< UserType > > _sequences;

      /** The device from (/to) which to perform the DMA transfer */
      boost::shared_ptr<DeviceBackend> _ioDevice;

      /** number of data blocks / samples */
      size_t _nBlocks;

  };

  /********************************************************************************************************************/

  template<class UserType>
  std::vector<UserType> & MultiplexedDataAccessor<UserType>::operator[](
      size_t sequenceIndex){
      return _sequences[sequenceIndex];
  }

  /********************************************************************************************************************/

  template<class UserType>
  MultiplexedDataAccessor<UserType>::MultiplexedDataAccessor( boost::shared_ptr< DeviceBackend > const & ioDevice )
  : _ioDevice(ioDevice), _nBlocks(0)
  {}

}  //namespace mtca4u

#endif // _MTCA4U_SEQUENCE_DE_MULTIPLEXER_H_
