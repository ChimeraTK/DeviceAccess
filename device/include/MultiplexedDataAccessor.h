#ifndef MTCA4U_MULTIPLEXED_DATA_ACCESSOR_H
#define MTCA4U_MULTIPLEXED_DATA_ACCESSOR_H

// @todo enable warning after a depcrecation warning for MultiplexedDataAccessor etc. has been released.
//#warning Including MultiplexDataAccessor.h is deprecated, include RegisterAccessor2D.h instead.
#include "TwoDRegisterAccessorImpl.h"

namespace mtca4u {

  template<typename UserType>
  class MultiplexedDataAccessorCopied;

  // Class backwards compatibility only. DEPCRECATED, DO NOT USE
  // @todo add printed warning after release of 0.6
  template<typename UserType>
  class MultiplexedDataAccessor : public TwoDRegisterAccessorImpl<UserType> {

    public:

      MultiplexedDataAccessor( boost::shared_ptr< DeviceBackend > const & ioDevice )
      :TwoDRegisterAccessorImpl<UserType>(ioDevice)
      {}

      MultiplexedDataAccessor( boost::shared_ptr< DeviceBackend > const & ioDevice,
          const std::vector<mtca4u::FixedPointConverter>& /*converters*/)
      :TwoDRegisterAccessorImpl<UserType>(ioDevice)
      {}

      /** \deprecated
       * Do not use, only for backwards compatibility.
       * A factory function which parses the register mapping and determines the
       *  correct type of SequenceDeMultiplexer.
       */
      static boost::shared_ptr< MultiplexedDataAccessor<UserType> > createInstance(
          std::string const & multiplexedSequenceName,
          std::string const & moduleName,
          boost::shared_ptr< DeviceBackend > const & ioDevice,
          boost::shared_ptr< RegisterInfoMap > const & /*registerMapping*/ )
      {
        return boost::shared_ptr< MultiplexedDataAccessor<UserType> >(
            new MultiplexedDataAccessorCopied<UserType>(ioDevice->getTwoDRegisterAccessor<UserType>(multiplexedSequenceName, moduleName) ));
      }

  };

  // Class backwards compatibility only. DEPCRECATED, DO NOT USE
  // @todo remove when MultiplexedDataAccessor is removed.
  template<typename UserType>
  class MultiplexedDataAccessorCopied : public MultiplexedDataAccessor<UserType> {

    public:

      MultiplexedDataAccessorCopied( boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > _accessor )
      : MultiplexedDataAccessor<UserType>(_accessor->_ioDevice),
        accessor(_accessor)
      {}

      /** Read the data from the device, de-multiplex the hardware IO buffer and
       *  fill the sequence buffers using the fixed point converters. The read
       *  method will handle reads into the DMA regions as well
       */
      virtual void read() {
        accessor->read();
        MultiplexedDataAccessor<UserType>::_sequences = accessor->_sequences;
      }

      /** Multiplex the data from the sequence buffer into the hardware IO buffer,
       * using the fixed point converters, and write it to the device. Can be used
       * to write to DMA memory Areas, but this functionality has not been
       * implemented yet
       */
      virtual void write() {
        accessor->_sequences = MultiplexedDataAccessor<UserType>::_sequences;
        accessor->write();
      }

      /**
       * Return the number of sequences that have been Multiplexed
       */
      virtual size_t getNumberOfDataSequences() {
        return accessor->getNumberOfDataSequences();
      }

      /**
       * Default destructor
       */
      virtual ~MultiplexedDataAccessorCopied() {};

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {// LCOV_EXCL_LINE
        return accessor->isSameRegister(other);// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      virtual bool isReadOnly() const {// LCOV_EXCL_LINE
        return accessor->isReadOnly();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

    protected:

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {// LCOV_EXCL_LINE
        return accessor->getHardwareAccessingElements();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {// LCOV_EXCL_LINE
        if(accessor->isSameRegister(newElement)) {// LCOV_EXCL_LINE
          accessor = boost::dynamic_pointer_cast< TwoDRegisterAccessorImpl<UserType> >(newElement);// LCOV_EXCL_LINE
        }// LCOV_EXCL_LINE
        else {// LCOV_EXCL_LINE
          accessor->replaceTransferElement(newElement);// LCOV_EXCL_LINE
        }// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > accessor;

  };

}

#endif /* MTCA4U_MULTIPLEXED_DATA_ACCESSOR_H */
