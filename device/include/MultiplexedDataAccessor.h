#ifndef CHIMERA_TK_MULTIPLEXED_DATA_ACCESSOR_H
#define CHIMERA_TK_MULTIPLEXED_DATA_ACCESSOR_H

// @todo enable warning after a depcrecation warning for MultiplexedDataAccessor etc. has been released.
//#warning Including MultiplexDataAccessor.h is deprecated, include RegisterAccessor2D.h instead.
#include "NDRegisterAccessor.h"
#include "DeviceBackend.h"

namespace ChimeraTK {

  // Class backwards compatibility only. DEPCRECATED, DO NOT USE
  // @todo change runtime warning into error after release of 0.9
  template<typename UserType>
  class MultiplexedDataAccessor : public NDRegisterAccessor<UserType> {

    public:

      MultiplexedDataAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : NDRegisterAccessor<UserType>(_accessor->getName(), _accessor->getUnit(), _accessor->getDescription()), accessor(_accessor)
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

      FixedPointConverter getFixedPointConverter() const override {
        throw ChimeraTK::logic_error("Deprecated and not implemented.");
      }

      void doReadTransfer() override {
        accessor->read();
      }

      bool doReadTransferNonBlocking() override {
        return accessor->readNonBlocking();
      }

      bool doReadTransferLatest() override {
        return accessor->readLatest();
      }

      void doPostRead() override {
        accessor->postRead();
        accessor->buffer_2D.swap(NDRegisterAccessor<UserType>::buffer_2D);
      }

      void doPreWrite() override {
        accessor->buffer_2D.swap(NDRegisterAccessor<UserType>::buffer_2D);
        accessor->preWrite();
      }

      void doPostWrite() override {
        accessor->postWrite();
        accessor->buffer_2D.swap(NDRegisterAccessor<UserType>::buffer_2D);
      }

      ChimeraTK::TransferFuture doReadTransferAsync() override {
        throw ChimeraTK::logic_error("Deprecated MultiplexedDataAccessor does not implement doReadTransferAsync().");
      }

      bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override {
        return accessor->doWriteTransfer(versionNumber);
      }

      bool isReadOnly() const override {// LCOV_EXCL_LINE
        return accessor->isReadOnly();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      bool isReadable() const override {// LCOV_EXCL_LINE
        return accessor->isReadable();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      bool isWriteable() const override{// LCOV_EXCL_LINE
        return accessor->isWriteable();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      AccessModeFlags getAccessModeFlags() const override { return{}; }

    protected:

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {// LCOV_EXCL_LINE
        return accessor->getHardwareAccessingElements();// LCOV_EXCL_LINE
      }// LCOV_EXCL_LINE

      std::list< boost::shared_ptr<TransferElement> > getInternalElements() override {
        return {};
      }

      boost::shared_ptr< NDRegisterAccessor<UserType> > accessor;

  };


}

#endif /* CHIMERA_TK_MULTIPLEXED_DATA_ACCESSOR_H */
