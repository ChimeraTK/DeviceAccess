/*
 * LNMBackendBufferingChannelAccessor.h
 *
 *  Created on: Feb 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H
#define MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H

#include <algorithm>

#include "LogicalNameMappingBackend.h"
#include "TwoDRegisterAccessor.h"
#include "TwoDRegisterAccessorImpl.h"
#include "FixedPointConverter.h"
#include "Device.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/

  template< typename T >
  class LNMBackendBufferingChannelAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      LNMBackendBufferingChannelAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess)
      : _registerPathName(registerPathName)
      {
        if(wordOffsetInRegister != 0 || numberOfWords > 1 || enforceRawAccess != false) {
          throw DeviceException("LNMBackendBufferingChannelAccessor: raw access, offset and number of words not yet "
              "supported!", DeviceException::NOT_IMPLEMENTED); // LCOV_EXCL_LINE (impossible to test...)
        }
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // copy the register info and create the internal accessors, if needed
        _info = *( boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(_registerPathName)) );
        _info.createInternalAccessors(dev);
        // check for incorrect usage of this accessor
        if( _info.targetType != LogicalNameMap::TargetType::CHANNEL ) {
          throw DeviceException("LNMBackendBufferingChannelAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        _targetDevice = _dev->_devices[_info.deviceName];
        _accessor = _targetDevice->getTwoDRegisterAccessor<T>("",_info.registerName);
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(_accessor.getNumberOfSamples());
      }

      virtual ~LNMBackendBufferingChannelAccessor() {};

      virtual void read() {
        _accessor.read();
        postRead();
      }

      virtual void write() {
        throw DeviceException("Writing to channel-type registers of logical name mapping devices is not supported.",
            DeviceException::REGISTER_IS_READ_ONLY);
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendBufferingChannelAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return true;
      }

      virtual FixedPointConverter getFixedPointConverter() const {
        throw DeviceException("Not implemented", DeviceException::NOT_IMPLEMENTED);
      }

    protected:

      /// pointer to underlying accessor
      TwoDRegisterAccessor<T> _accessor;

      /// register and module name
      RegisterPath _registerPathName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information. We hold a copy of the RegisterInfo, since it might contain register accessors
      /// which may not be owned by the backend
      LogicalNameMap::RegisterInfo _info;

      /// target device
      boost::shared_ptr<Device> _targetDevice;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor.getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        _accessor.replaceTransferElement(newElement);
      }

      virtual void postRead() {
        _accessor.postRead();
        _accessor[_info.channel].swap(BufferingRegisterAccessorImpl<T>::cookedBuffer);
      };

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H */
