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
      : impl(ndRegisterAccessor), _direction(direction), _mode(mode)
      {}

    public:

      void set(ChimeraTK::ProcessScalar<UserType> const & other) {
        impl->accessData(0) = other.get();
      }

      void set(UserType const & t) {
        impl->accessData(0) = t;
      }

      operator UserType() const {
        return impl->accessData(0);
      }

      UserType get() const {
        return impl->accessData(0);
      }

      const std::type_info& getValueType() const {
        return typeid(UserType);
      }

      bool isReadable() const {
        return _direction == VariableDirection::consuming;
      }
      
      bool isReadOnly() const {
        return _direction == VariableDirection::consuming;
      }
      
      bool isWriteable() const {
        return _direction == VariableDirection::feeding;
      }

      TimeStamp getTimeStamp() const {
        return TimeStamp();
      }

      bool readNonBlocking() {
        assert(_direction == VariableDirection::consuming);
        if(impl->getNInputQueueElements() == 0) return false;
        impl->read();
        return true;
      }
      
      void read(){
        throw std::logic_error("Blocking read is not supported by process array.");
      }
      
      void write() {
        assert(_direction == VariableDirection::feeding);
        impl->write();
      }
      
      bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const{
        // only true if the very instance of the transfer element is the same
        return e.get() == this;
      }
      
      std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements(){
        return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
      }
      
      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>){
        // You can't replace anything here. Just do nothing.
      }
      
    protected:

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;
      VariableDirection _direction;
      UpdateMode _mode;
 };


} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_ACCESSOR_H */
