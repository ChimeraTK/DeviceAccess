#ifndef MTCA4U_MULTIPLEXED_DATA_ACCESSOR_H
#define MTCA4U_MULTIPLEXED_DATA_ACCESSOR_H

// @todo enable warning after a depcrecation warning for MultiplexedDataAccessor etc. has been released.
//#warning Including MultiplexDataAccessor.h is deprecated, include RegisterAccessor2D.h instead.
#include "NDRegisterAccessor.h"
#include "DeviceBackend.h"

namespace mtca4u {

  // Class backwards compatibility only. DEPCRECATED, DO NOT USE
  // @todo change runtime warning into error after release of 0.9
  template<typename UserType>
  class MultiplexedDataAccessor : public NDRegisterAccessor<UserType> {

    public:

      MultiplexedDataAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : accessor(_accessor)
      {
        std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
        std::cerr << "** Usage of deprecated class MultiplexedDataAccessor detected.                                 **" << std::endl;// LCOV_EXCL_LINE
        std::cerr << "** Use TwoDRegisterAccessor instead!                                                           **" << std::endl;// LCOV_EXCL_LINE
        std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
        MultiplexedDataAccessor<UserType>::buffer_2D = accessor->buffer_2D;
      }

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
        return boost::shared_ptr< MultiplexedDataAccessor<UserType> >(new MultiplexedDataAccessor<UserType>(
            ioDevice->getRegisterAccessor<UserType>(RegisterPath(moduleName)/multiplexedSequenceName,0,0,false)
            ));
      }

      /** \deprecated
       * Do not use, only for backwards compatibility.
       * Redefine the user type, as we need it for the backwards-compatible factory Device::getCustomAccessor()
       */
      typedef UserType userType;

      /** Operator to access individual sequences.
       */
      std::vector<UserType> & operator[](size_t sequenceIndex) {
        return NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex];
      }

      size_t getNumberOfDataSequences() const {
        return NDRegisterAccessor<UserType>::getNumberOfChannels();
      }

      FixedPointConverter getFixedPointConverter() const {
        throw DeviceException("Deprecated and not implemented.", DeviceException::NOT_IMPLEMENTED);
      }

      void doReadTransfer() override {
        accessor->read();
      }

      bool doReadTransferNonBlocking() override {
        return accessor->readNonBlocking();
      }
      
      void postRead() override {
        accessor->buffer_2D.swap(NDRegisterAccessor<UserType>::buffer_2D);
      }

      /** Multiplex the data from the sequence buffer into the hardware IO buffer,
       * using the fixed point converters, and write it to the device. Can be used
       * to write to DMA memory Areas, but this functionality has not been
       * implemented yet
       */
      virtual void write() {
        accessor->buffer_2D.swap(NDRegisterAccessor<UserType>::buffer_2D);
        accessor->write();
      }

      /**
       * Default destructor
       */
      virtual ~MultiplexedDataAccessor() {}

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {// LCOV_EXCL_LINE
        return accessor->isSameRegister(other);// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      virtual bool isReadOnly() const {// LCOV_EXCL_LINE
        return accessor->isReadOnly();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      virtual bool isReadable() const {// LCOV_EXCL_LINE
        return accessor->isReadable();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      virtual bool isWriteable() const {// LCOV_EXCL_LINE
        return accessor->isWriteable();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

    protected:

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {// LCOV_EXCL_LINE
        return accessor->getHardwareAccessingElements();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {// LCOV_EXCL_LINE
        if(accessor->isSameRegister(newElement)) {// LCOV_EXCL_LINE
          accessor = boost::dynamic_pointer_cast< NDRegisterAccessor<UserType> >(newElement);// LCOV_EXCL_LINE
        }// LCOV_EXCL_LINE
        else {// LCOV_EXCL_LINE
          accessor->replaceTransferElement(newElement);// LCOV_EXCL_LINE
        }// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      boost::shared_ptr< NDRegisterAccessor<UserType> > accessor;

  };


}

#endif /* MTCA4U_MULTIPLEXED_DATA_ACCESSOR_H */
