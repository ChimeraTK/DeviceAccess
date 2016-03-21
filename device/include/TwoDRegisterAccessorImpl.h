#ifndef MTCA4U_TWO_D_REGISTER_ACCESSOR_IMPL_H
#define MTCA4U_TWO_D_REGISTER_ACCESSOR_IMPL_H

#include <sstream>
#include <boost/shared_ptr.hpp>

#include "RegisterInfoMap.h"
#include "FixedPointConverter.h"
#include "DeviceBackend.h"
#include "MapException.h"
#include "NotImplementedException.h"
#include "TransferElement.h"

namespace mtca4u{

  /** Exception class for MultiplexedDataAccessor
   */
  class TwoDRegisterAccessorException : public Exception {
    public:

      enum { EMPTY_AREA, INVALID_WORD_SIZE, INVALID_N_ELEMENTS };

      TwoDRegisterAccessorException(const std::string &message, unsigned int ID)
      : Exception(message, ID)
      {}
  };

  /// @todo compatibility typedef, add printed warning after release of version 0.6
  typedef TwoDRegisterAccessorException MultiplexedDataAccessorException;

  /// for compatibility only
  template<typename UserType>
  class MultiplexedDataAccessorCopied;

  /// forward declaration to make a friend
  template<class UserType>
  class TwoDRegisterAccessor;

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessorImpl : public TransferElement {
    public:
      /** Constructor to intialise the members.
       */
      TwoDRegisterAccessorImpl( boost::shared_ptr< DeviceBackend > const & ioDevice );

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
      virtual size_t getNumberOfDataSequences() const = 0;

      /** Return number of samples per sequence (=channel).
       *  This function assumes that the number of samples is equal for all sequences. If an implementation supports
       *  different number of samples for each sequence, it needs to override this function and throw an exception.
       *
       *  Implementation note: The default implementation will return the length of the sequence 0. If in future
       *  implementations with different number of samples for each sequence are present, this function needs to be
       *  extended with an optional argument of the channel number. Only if called without specifying the argument
       *  should throw an exception then.
       */
      virtual unsigned int getNumberOfSamples() const;

      /**
       * Default destructor
       */
      virtual ~TwoDRegisterAccessorImpl() {};

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

      // for compatibility onlye
      friend class MultiplexedDataAccessorCopied<UserType>;

      /// the public interface needs access to protected functions of the TransferElement etc.
      friend class TwoDRegisterAccessor<UserType>;

  };

  /********************************************************************************************************************/

  template<class UserType>
  std::vector<UserType> & TwoDRegisterAccessorImpl<UserType>::operator[](
      size_t sequenceIndex){
      return _sequences[sequenceIndex];
  }

  /********************************************************************************************************************/

  template<class UserType>
  unsigned int TwoDRegisterAccessorImpl<UserType>::getNumberOfSamples() const {
    return _sequences[0].size();
  }

  /********************************************************************************************************************/

  template<class UserType>
  TwoDRegisterAccessorImpl<UserType>::TwoDRegisterAccessorImpl( boost::shared_ptr< DeviceBackend > const & ioDevice )
  : _ioDevice(ioDevice), _nBlocks(0)
  {}

}  //namespace mtca4u

#endif // MTCA4U_TWO_D_REGISTER_ACCESSOR_IMPL_H
