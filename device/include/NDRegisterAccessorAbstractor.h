/*
 * NDRegisterAccessorAbstractor.h
 *
 *  Created on: Mar 23, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_N_D_REGISTER_ACCESSOR_ABSTRACTOR_H
#define MTCA4U_N_D_REGISTER_ACCESSOR_ABSTRACTOR_H

#include "ForwardDeclarations.h"
#include "TransferElementAbstractor.h"
#include "NDRegisterAccessor.h"
#include "CopyRegisterDecorator.h"

namespace mtca4u {

  /** Base class for the reigster accessor abstractors (ScalarRegisterAccessor, OneDRegisterAccessor and
   *  TwoDRegisterAccessor). Provides a private implementation of the TransferElement interface to allow the bridges
   *  to be added to a TransferGroup. Also stores the shared pointer to the NDRegisterAccessor implementation. */
  template<typename UserType>
  class NDRegisterAccessorAbstractor : public TransferElementAbstractor {

    public:

      /** Return number of waiting data elements in the queue (or buffer). Use when the accessor was obtained with
       *  AccessMode::wait_for_new_data to obtain the amount of data waiting for retrieval in this accessor. If the
       *  returned value is 0, the call to read() will block until new data has arrived. If the returned value is > 0,
       *  it is guaranteed that the next call to read() will not block. If the accessor was obtained without the
       *  AccessMode::wait_for_new_data flag, this function will always return 1. */
      unsigned int getNInputQueueElements() const {
        return _impl->getNInputQueueElements();
      }

      /** Assign a new accessor to this NDRegisterAccessorAbstractor. Since another NDRegisterAccessorAbstractor is passed as
       *  argument, both NDRegisterAccessorAbstractors will then point to the same accessor and thus are sharing the
       *  same buffer. To obtain a new copy of the accessor with a distinct buffer, the corresponding
       *  getXXRegisterAccessor() function of Device must be called. */
      void replace(const NDRegisterAccessorAbstractor<UserType> &newAccessor) {
        _impl = newAccessor._impl;
        TransferElementAbstractor::_id = _impl->getId();
      }

      /** Alternative signature of relace() with the same functionality, used when a pointer to the implementation
       *  has been obtained directly (instead of a NDRegisterAccessorAbstractor). */
      void replace(boost::shared_ptr<NDRegisterAccessor<UserType>> newImpl) {
        _impl = newImpl;
        TransferElementAbstractor::_id = _impl->getId();
      }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl != NULL;
      }

      void read() override {
        NDRegisterAccessorAbstractor<UserType>::_impl->read();
      }

      bool readNonBlocking() override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->readNonBlocking();
      }

      bool readLatest() override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->readLatest();
      }

      void doReadTransfer() override {
        NDRegisterAccessorAbstractor<UserType>::_impl->doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->doReadTransferNonBlocking();
      }

      bool doReadTransferLatest() override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->doReadTransferLatest();
      }

      void doPreRead() override {
        NDRegisterAccessorAbstractor<UserType>::_impl->preRead();
      }

      void doPostRead() override {
        NDRegisterAccessorAbstractor<UserType>::_impl->postRead();
      }

      void doPreWrite() override {
        NDRegisterAccessorAbstractor<UserType>::_impl->preWrite();
      }

      void doPostWrite() override {
        NDRegisterAccessorAbstractor<UserType>::_impl->postWrite();
      }

      TransferFuture readAsync() override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->readAsync();
      }

      bool asyncTransferActive() override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->asyncTransferActive();
      }

      ChimeraTK::VersionNumber getVersionNumber() const override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getVersionNumber();
      }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->write(versionNumber);
      }

      bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->doWriteTransfer(versionNumber);
      }

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->isReadOnly();
      }

      bool isReadable() const override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->isReadable();
      }

      bool isWriteable() const override {
        return NDRegisterAccessorAbstractor<UserType>::_impl->isWriteable();
      }

      bool mayReplaceOther(const boost::shared_ptr<TransferElement const> &other) const override {
        return _impl->mayReplaceOther(other);
      }

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
        return _impl->getHardwareAccessingElements();
      }

      std::list< boost::shared_ptr<TransferElement> > getInternalElements() override {
        auto result = _impl->getInternalElements();
        result.push_front(_impl);
        return result;
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        if(newElement->mayReplaceOther(_impl)) {
          if(newElement != _impl) {
            auto casted = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(newElement);
            _impl = boost::make_shared<ChimeraTK::CopyRegisterDecorator<UserType>>(casted);
          }
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
        TransferElementAbstractor::_id = _impl->getId();
      }

      boost::shared_ptr<TransferElement> getHighLevelImplElement() override {
        return _impl;
      }

      const std::type_info& getValueType() const override {
        return typeid(UserType);
      }

      void clearAsyncTransferActive() override {
        throw std::logic_error("NDRegisterAccessorAbstractor::clearAsyncTransferActive(): Users should never call this function!");
      }

      void transferFutureWaitCallback() override {
        throw std::logic_error("NDRegisterAccessorAbstractor::clearAsyncTransferActive(): Users should never call this function!");
      }

    protected:

      NDRegisterAccessorAbstractor(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : _impl(impl)
      {
        TransferElementAbstractor::_id = _impl->getId();
      }

      NDRegisterAccessorAbstractor() {}

      /** pointer to the implementation */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _impl;

      //friend class TransferGroup;

    private:

      /** prevent copying by operator=, since it will be confusing (operator= may also be overloaded to access the
       *  content of the buffer!) */
      const NDRegisterAccessorAbstractor& operator=(const NDRegisterAccessorAbstractor& rightHandSide) const;

  };
}

#endif /* MTCA4U_N_D_REGISTER_ACCESSOR_BRIDGE_H */
