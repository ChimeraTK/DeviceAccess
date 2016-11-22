/*
 * LNMBackendBufferingChannelAccessor.h
 *
 *  Created on: Feb 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H
#define MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H

#include <algorithm>

#include "NDRegisterAccessor.h"
#include "LogicalNameMappingBackend.h"
#include "TwoDRegisterAccessor.h"
#include "FixedPointConverter.h"
#include "Device.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendChannelAccessor : public NDRegisterAccessor<UserType> {
    public:

      LNMBackendChannelAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
      : _registerPathName(registerPathName)
      {
        // check for unknown flags
        flags.checkForUnknownFlags({AccessMode::raw});
        // check for illegal parameter combinations
        if(wordOffsetInRegister != 0 || numberOfWords > 1 || flags.has(AccessMode::raw)) {
          throw DeviceException("LNMBackendBufferingChannelAccessor: raw access, offset and number of words not yet "
              "supported!", DeviceException::NOT_IMPLEMENTED); // LCOV_EXCL_LINE (impossible to test...)
        }
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // copy the register info and create the internal accessors, if needed
        _info = *( boost::static_pointer_cast<LNMBackendRegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(_registerPathName)) );
        _info.createInternalAccessors(dev);
        // check for incorrect usage of this accessor
        if( _info.targetType != LNMBackendRegisterInfo::TargetType::CHANNEL ) {
          throw DeviceException("LNMBackendBufferingChannelAccessor used for wrong register type.",
              DeviceException::WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        // get target device and accessor
        _targetDevice = _dev->_devices[_info.deviceName];
        _accessor = _targetDevice->getRegisterAccessor<UserType>(RegisterPath(_info.registerName), 0,0, false);
        // allocate the buffer
        NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(_accessor->getNumberOfSamples());
      }

      virtual ~LNMBackendChannelAccessor() {};

      virtual void read() {
        if(TransferElement::isInTransferGroup) {
          throw DeviceException("Calling read() or write() on an accessor which is part of a TransferGroup is not allowed.",
              DeviceException::NOT_IMPLEMENTED);
        }
        _accessor->read();
        postRead();
      }

      virtual void write() {
        throw DeviceException("Writing to channel-type registers of logical name mapping devices is not supported.",
            DeviceException::REGISTER_IS_READ_ONLY);
      }

      virtual bool readNonBlocking(){
         throw DeviceException("Non-blocking read is not implemented yet for the LNMBackendChannelAccessor",
                               DeviceException::NOT_IMPLEMENTED);
      }


      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendChannelAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return true;
      }

      virtual bool isReadable() const {
        return true;
      }

      virtual bool isWriteable() const {
        return false;
      }

      virtual FixedPointConverter getFixedPointConverter() const {
        throw DeviceException("FixedPointConverterse are not available in Logical Name Mapping",
                              DeviceException::NOT_AVAILABLE);
      }

    protected:

      /// pointer to underlying accessor
      boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor;

      /// register and module name
      RegisterPath _registerPathName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information. We hold a copy of the RegisterInfo, since it might contain register accessors
      /// which may not be owned by the backend
      LNMBackendRegisterInfo _info;

      /// target device
      boost::shared_ptr<DeviceBackend> _targetDevice;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        auto casted = boost::dynamic_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        if(newElement->isSameRegister(_accessor) && casted) {
          _accessor = casted;
        }
        else {
          _accessor->replaceTransferElement(newElement);
        }
      }

      virtual void postRead() {
        _accessor->postRead();
        _accessor->accessChannel(_info.channel).swap(NDRegisterAccessor<UserType>::buffer_2D[0]);
      };

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H */
