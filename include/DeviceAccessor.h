/*
 * DeviceAccessor.h
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEVICE_ACCESSOR_H
#define CHIMERATK_DEVICE_ACCESSOR_H

#include <mtca4u/NDRegisterAccessor.h>

#include "Accessor.h"

namespace ChimeraTK {

  /** A DeviceAccessor is used to provide access to register accessors from the DeviceAccess library. It is a temporary
   *  adapter class which harmonises the different interfaces. @todo TODO Once the control system adapter and the
   *  DeviceAccess interfaces have been harmonised, this class is no longer needed! */
  template< typename UserType >
  class DeviceAccessor : public mtca4u::NDRegisterAccessor<UserType> {
    public:

      /**  */
      DeviceAccessor(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> ndRegisterAccessor, VariableDirection direction,
          UpdateMode mode)
      : mtca4u::NDRegisterAccessor<UserType>(ndRegisterAccessor->getName()),
        impl(ndRegisterAccessor),
        _direction(direction),
        _mode(mode)
      {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize( impl->getNumberOfChannels() );
        for(size_t i=0; i<impl->getNumberOfChannels(); i++) {
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D[i].resize( impl->getNumberOfSamples() );
        }
      }
      
      void read() {
        impl->read();
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(impl->accessChannel(0));
      }
      
      bool readNonBlocking() {
        impl->read();           /// @todo FIXME this is wrong, but otherwise DeviceAccess fails to work properly right now
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(impl->accessChannel(0));
        return true;
      }
      
      void write() {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(impl->accessChannel(0));
        impl->write();
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(impl->accessChannel(0));
      }
      
      unsigned int getNInputQueueElements() const {
        return impl->getNInputQueueElements();
      }
      
      bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& other) const {
        return impl->isSameRegister(other);
      }
      
      bool isReadOnly() const {
        return impl->isReadOnly();
      }
      
      bool isReadable() const {
        return impl->isReadable();
      }
      
      bool isWriteable() const {
        return impl->isWriteable();
      }
      
      std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() {
        return impl->getHardwareAccessingElements();
      }
      
      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement> other) {
        impl->replaceTransferElement(other);
      }
      
  protected:

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;
      VariableDirection _direction;
      UpdateMode _mode;
 };


} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_ACCESSOR_H */
