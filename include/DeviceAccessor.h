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
  class DeviceAccessor : public mtca4u::ProcessScalar<UserType> {
    public:

      /**  */
      DeviceAccessor(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> ndRegisterAccessor, VariableDirection direction,
          UpdateMode mode)
      : impl(ndRegisterAccessor), _direction(direction), _mode(mode)
      {}

    public:

      void set(mtca4u::ProcessScalar<UserType> const & other) {
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

      bool isReceiver() const {
        return _direction == VariableDirection::input;
      }

      bool isSender() const {
        return _direction == VariableDirection::output;
      }

      TimeStamp getTimeStamp() const {
        return TimeStamp();
      }

      bool receive() {
        if(impl->getNInputQueueElements() == 0) return false;
        impl->read();
        return true;
      }

      bool send() {
        impl->write();
        return true;
      }

    protected:

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;
      VariableDirection _direction;
      UpdateMode _mode;
 };


} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_ACCESSOR_H */
