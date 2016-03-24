/*
 * LNMBackendBufferingRegisterAccessor.h
 *
 *  Created on: Feb 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LNM_BACKEND_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_LNM_BACKEND_BUFFERING_REGISTER_ACCESSOR_H

#include <algorithm>

#include "LogicalNameMappingBackend.h"
#include "BufferingRegisterAccessor.h"
#include "FixedPointConverter.h"
#include "Device.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/

  template<typename T>
  class LNMBackendBufferingRegisterAccessor : public NDRegisterAccessor<T> {
    public:

      LNMBackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess)
      : _registerPathName(registerPathName)
      {
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // copy the register info and create the internal accessors, if needed
        _info = *( boost::static_pointer_cast<LogicalNameMapParser::RegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(registerPathName)) );
        _info.createInternalAccessors(dev);
        // check for incorrect usage of this accessor
        if( _info.targetType != LogicalNameMapParser::TargetType::RANGE &&
            _info.targetType != LogicalNameMapParser::TargetType::REGISTER ) {
          throw DeviceException("LNMBackendBufferingRegisterAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        // obtain target device pointer
        _targetDevice = _dev->_devices[_info.deviceName];
        // compute actual length and offset
        if(_info.targetType == LogicalNameMapParser::TargetType::REGISTER) {
          _info.firstIndex = 0;
          _info.length = 0;
        }
        actualOffset = _info.firstIndex + wordOffsetInRegister;
        actualLength = ( numberOfWords > 0 ? numberOfWords : _info.length );
        // obtain underlying register accessor
        _accessor = _targetDevice->getRegisterAccessor<T>(RegisterPath(_info.registerName),
            actualLength,actualOffset,enforceRawAccess);
        if(actualLength == 0) actualLength = _accessor->getNumberOfSamples();
        // create buffer
        NDRegisterAccessor<T>::buffer_2D.resize(1);
        NDRegisterAccessor<T>::buffer_2D[0].resize(actualLength);
      }

      virtual ~LNMBackendBufferingRegisterAccessor() {};

      virtual void read() {
        _accessor->read();
        postRead();
      }

      virtual void write() {
        if(isReadOnly()) {
          throw DeviceException("Writing to range-type registers of logical name mapping devices is not supported.",
              DeviceException::REGISTER_IS_READ_ONLY);
        }
        preWrite();
        _accessor->write();
        postWrite();
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendBufferingRegisterAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(actualLength != rhsCasted->actualLength) return false;
        if(actualOffset != rhsCasted->actualOffset) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        if(_info.targetType == LogicalNameMapParser::TargetType::RANGE) {
          return true;
        }
        else {
          return false;
        }
      }

      virtual FixedPointConverter getFixedPointConverter() const {
        throw DeviceException("Not implemented", DeviceException::NOT_IMPLEMENTED);
      }

    protected:

      /// pointer to underlying accessor
      boost::shared_ptr< NDRegisterAccessor<T> > _accessor;

      /// register and module name
      RegisterPath _registerPathName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information. We hold a copy of the RegisterInfo, since it might contain register accessors
      /// which may not be owned by the backend
      LogicalNameMapParser::RegisterInfo _info;

      /// target device
      boost::shared_ptr<DeviceBackend> _targetDevice;

      /// actual length and offset w.r.t. beginning of the underlying register
      size_t actualLength, actualOffset;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        auto casted = boost::dynamic_pointer_cast< NDRegisterAccessor<T> >(newElement);
        if(newElement->isSameRegister(_accessor) && casted) {
          _accessor = casted;
        }
        else {
          _accessor->replaceTransferElement(newElement);
        }
      }

      virtual void postRead() {
        _accessor->postRead();
        _accessor->accessChannel(0).swap(NDRegisterAccessor<T>::buffer_2D[0]);
      };

      virtual void preWrite() {
        _accessor->preWrite();
        _accessor->accessChannel(0).swap(NDRegisterAccessor<T>::buffer_2D[0]);
      };

      virtual void postWrite() {
        _accessor->postWrite();
        _accessor->accessChannel(0).swap(NDRegisterAccessor<T>::buffer_2D[0]);
      };

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_REGISTER_ACCESSOR_H */
