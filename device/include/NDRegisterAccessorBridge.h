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
  class NDRegisterAccessorBridge : protected TransferElement {
    protected:

      NDRegisterAccessorBridge(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : _impl(impl)
      {}

      NDRegisterAccessorBridge() {}

      /** pointer to the implementation */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _impl;

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        return _impl->isSameRegister(other);
      }

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _impl->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_impl->isSameRegister(newElement)) {
          _impl = boost::dynamic_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
      }

      virtual boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return _impl;
      }

      friend class TransferGroup;

  };
}

#endif /* MTCA4U_N_D_REGISTER_ACCESSOR_BRIDGE_H */
