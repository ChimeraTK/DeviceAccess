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
#include "BufferingRegisterAccessorImpl.h"
#include "FixedPointConverter.h"
#include "Device.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/

  template<typename T>
  class LNMBackendBufferingRegisterAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      LNMBackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const std::string &module,
          const std::string &registerName)
      : _registerName(registerName),_moduleName(module)
      {
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        std::string name = ( module.length() > 0 ? module + "." + registerName : registerName );
        _info = _dev->_map.getRegisterInfo(name);
        if( _info.targetType != LogicalNameMap::TargetType::RANGE &&
            _info.targetType != LogicalNameMap::TargetType::REGISTER ) {
          throw DeviceException("LNMBackendBufferingRegisterAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER);
        }
        _targetDevice = _dev->_devices[_info.deviceName];
        _accessor = _targetDevice->getBufferingRegisterAccessor<T>("",_info.registerName);
        if(_info.targetType == LogicalNameMap::TargetType::REGISTER) {
          _info.firstIndex = 0;
          _info.length = _accessor.getNumberOfElements();
        }
        index_begin = _info.firstIndex;
        index_end = _info.firstIndex + _info.length;
        index_rbegin = _accessor.getNumberOfElements() - index_end;
        index_rend = _accessor.getNumberOfElements() - index_begin;
      }

      virtual ~LNMBackendBufferingRegisterAccessor() {};

      virtual void read() {
        _accessor.read();
      }

      virtual void write() {
        if(isReadOnly()) {
          throw DeviceException("Writing to range-type registers of logical name mapping devices is not supported.",
              DeviceException::REGISTER_IS_READ_ONLY);
        }
        _accessor.write();
      }

      virtual T& operator[](unsigned int index) {
        return _accessor[index + _info.firstIndex];
      }

      virtual unsigned int getNumberOfElements() {
        return _info.length;
      }

      typedef typename BufferingRegisterAccessorImpl<T>::iterator iterator;
      typedef typename BufferingRegisterAccessorImpl<T>::const_iterator const_iterator;
      typedef typename BufferingRegisterAccessorImpl<T>::reverse_iterator reverse_iterator;
      typedef typename BufferingRegisterAccessorImpl<T>::const_reverse_iterator const_reverse_iterator;
      virtual iterator begin() { return _accessor.begin() + index_begin; }
      virtual const_iterator begin() const { return _accessor.begin() + index_begin; }
      virtual iterator end() { return _accessor.begin() + index_end; }
      virtual const_iterator end() const { return _accessor.begin() + _info.firstIndex + _info.length; }
      virtual reverse_iterator rbegin() { return _accessor.rbegin() + index_rbegin; }
      virtual const_reverse_iterator rbegin() const { return _accessor.rbegin() + index_rbegin; }
      virtual reverse_iterator rend() { return _accessor.rbegin() + index_rend; }
      virtual const_reverse_iterator rend() const { return _accessor.rbegin() + index_rend; }

      virtual void swap(std::vector<T> &x) {
        std::swap_ranges( x.begin(), x.end(), begin() );
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendBufferingRegisterAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerName != rhsCasted->_registerName) return false;
        if(_moduleName != rhsCasted->_moduleName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        if(_info.targetType == LogicalNameMap::TargetType::RANGE) {
          return true;
        }
        else {
          return false;
        }
      }

    protected:

      /// pointer to underlying accessor
      BufferingRegisterAccessor<T> _accessor;

      /// register and module name
      std::string _registerName, _moduleName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information
      LogicalNameMap::RegisterInfo _info;

      /// target device
      boost::shared_ptr<Device> _targetDevice;

      /// indices needed to perpare the iterators
      int index_begin, index_end, index_rbegin, index_rend;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _accessor.getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        _accessor.replaceTransferElement(newElement);
      }

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_REGISTER_ACCESSOR_H */
