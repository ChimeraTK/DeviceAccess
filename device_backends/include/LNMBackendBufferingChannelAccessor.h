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

  template<typename T>
  class LNMBackendBufferingChannelAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      LNMBackendBufferingChannelAccessor(boost::shared_ptr<DeviceBackend> dev, const std::string &module,
          const std::string &registerName)
      : _registerName(registerName),_moduleName(module)
      {
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // copy the register info and create the internal accessors, if needed
        RegisterPath name = RegisterPath(module)/registerName;
        _info = *( boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(_dev->getRegisterCatalogue().getRegister(name)) );
        _info.createInternalAccessors(dev);
        // check for incorrect usage of this accessor
        if( _info.targetType != LogicalNameMap::TargetType::CHANNEL ) {
          throw DeviceException("LNMBackendBufferingChannelAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        _targetDevice = _dev->_devices[_info.deviceName];
        _accessor = _targetDevice->getTwoDRegisterAccessor<T>("",_info.registerName);
      }

      virtual ~LNMBackendBufferingChannelAccessor() {};

      virtual void read() {
        _accessor.read();
      }

      virtual void write() {
        throw DeviceException("Writing to channel-type registers of logical name mapping devices is not supported.",
            DeviceException::REGISTER_IS_READ_ONLY);
      }

      virtual T& operator[](unsigned int index) {
        return _accessor[_info.channel][index];
      }

      virtual unsigned int getNumberOfElements() {
        return _accessor[_info.channel].size();
      }

      typedef typename BufferingRegisterAccessorImpl<T>::iterator iterator;
      typedef typename BufferingRegisterAccessorImpl<T>::const_iterator const_iterator;
      typedef typename BufferingRegisterAccessorImpl<T>::reverse_iterator reverse_iterator;
      typedef typename BufferingRegisterAccessorImpl<T>::const_reverse_iterator const_reverse_iterator;
      virtual iterator begin() { return _accessor[_info.channel].begin(); }
      virtual const_iterator cbegin() const { return _accessor[_info.channel].cbegin(); }
      virtual iterator end() { return _accessor[_info.channel].end(); }
      virtual const_iterator cend() const { return _accessor[_info.channel].cend(); }
      virtual reverse_iterator rbegin() { return _accessor[_info.channel].rbegin(); }
      virtual const_reverse_iterator crbegin() const { return _accessor[_info.channel].crbegin(); }
      virtual reverse_iterator rend() { return _accessor[_info.channel].rend(); }
      virtual const_reverse_iterator crend() const { return _accessor[_info.channel].crend(); }

      virtual void swap(std::vector<T> &x) {
        _accessor[_info.channel].swap(x);
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendBufferingChannelAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerName != rhsCasted->_registerName) return false;
        if(_moduleName != rhsCasted->_moduleName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return true;
      }

    protected:

      /// pointer to underlying accessor
      TwoDRegisterAccessor<T> _accessor;

      /// register and module name
      std::string _registerName, _moduleName;

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

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_CHANNEL_ACCESSOR_H */
