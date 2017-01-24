/*
 * NDRegisterAccessorBridge.h
 *
 *  Created on: Mar 23, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_N_D_REGISTER_ACCESSOR_BRIDGE_H
#define MTCA4U_N_D_REGISTER_ACCESSOR_BRIDGE_H

#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "NDRegisterAccessor.h"

namespace mtca4u {

  /** Base class for the reigster accessor bridges (ScalarRegisterAccessor, OneDRegisterAccessor and
   *  TwoDRegisterAccessor). Provides a private implementation of the TransferElement interface to allow the bridges
   *  to be added to a TransferGroup. Also stores the shared pointer to the NDRegisterAccessor implementation. */
  template<typename UserType>
  class NDRegisterAccessorBridge : public TransferElement {

    public:

      /** Return number of waiting data elements in the queue (or buffer). Use when the accessor was obtained with
       *  AccessMode::wait_for_new_data to obtain the amount of data waiting for retrieval in this accessor. If the
       *  returned value is 0, the call to read() will block until new data has arrived. If the returned value is > 0,
       *  it is guaranteed that the next call to read() will not block. If the accessor was obtained without the
       *  AccessMode::wait_for_new_data flag, this function will always return 1. */
      unsigned int getNInputQueueElements() const {
        return _impl->getNInputQueueElements();
      }

      /** Assign a new accessor to this NDRegisterAccessorBridge. Since another NDRegisterAccessorBridge is passed as
       *  argument, both NDRegisterAccessorBridges will then point to the same accessor and thus are sharing the
       *  same buffer. To obtain a new copy of the accessor with a distinct buffer, the corresponding
       *  getXXRegisterAccessor() function of Device must be called. */
      void replace(const NDRegisterAccessorBridge &newAccessor) {
        _impl = newAccessor._impl;
      }

      void doReadTransfer() override {
        NDRegisterAccessorBridge<UserType>::_impl->doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        return NDRegisterAccessorBridge<UserType>::_impl->doReadTransferNonBlocking();
      }

      void postRead() override {
        NDRegisterAccessorBridge<UserType>::_impl->postRead();
      }

      void write() override {
        NDRegisterAccessorBridge<UserType>::_impl->write();
      }

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const override {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadOnly();
      }

      bool isReadable() const override {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadable();
      }

      bool isWriteable() const override {
        return NDRegisterAccessorBridge<UserType>::_impl->isWriteable();
      }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return NDRegisterAccessorBridge<UserType>::_impl != NULL;
      }

    protected:

      NDRegisterAccessorBridge(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : _impl(impl)
      {}

      NDRegisterAccessorBridge() {}

      /** pointer to the implementation */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _impl;

      bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const override {
        return _impl->isSameRegister(other);
      }

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
        return _impl->getHardwareAccessingElements();
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        if(_impl->isSameRegister(newElement)) {
          _impl = boost::dynamic_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
      }

      boost::shared_ptr<TransferElement> getHighLevelImplElement() override {
        return _impl;
      }

      friend class TransferGroup;

      const std::type_info& getValueType() const override {
        return typeid(UserType);
      }

    private:

      /** prevent copying by operator=, since it will be confusing (operator= may also be overloaded to access the
       *  content of the buffer!) */
      const NDRegisterAccessorBridge& operator=(const NDRegisterAccessorBridge& rightHandSide) const;

  };
}

#endif /* MTCA4U_N_D_REGISTER_ACCESSOR_BRIDGE_H */
