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

      LNMBackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess)
      : _registerPathName(registerPathName)
      {
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // copy the register info and create the internal accessors, if needed
        _info = *( boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(registerPathName)) );
        _info.createInternalAccessors(dev);
        // check for incorrect usage of this accessor
        if( _info.targetType != LogicalNameMap::TargetType::RANGE &&
            _info.targetType != LogicalNameMap::TargetType::REGISTER ) {
          throw DeviceException("LNMBackendBufferingRegisterAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        _targetDevice = _dev->_devices[_info.deviceName];
        _accessor = _targetDevice->getBufferingRegisterAccessor<T>(RegisterPath(_info.registerName),
            0,0,enforceRawAccess);
        if(_info.targetType == LogicalNameMap::TargetType::REGISTER) {
          _info.firstIndex = 0;
          _info.length = _accessor.getNumberOfElements();
        }
        _info.firstIndex = _info.firstIndex + wordOffsetInRegister;
        if(numberOfWords > 0) _info.length = numberOfWords;
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
      virtual const_iterator cbegin() const { return _accessor.cbegin() + index_begin; }
      virtual iterator end() { return _accessor.begin() + index_end; }
      virtual const_iterator cend() const { return _accessor.cbegin() + index_end; }
      virtual reverse_iterator rbegin() { return _accessor.rbegin() + index_rbegin; }
      virtual const_reverse_iterator crbegin() const { return _accessor.crbegin() + index_rbegin; }
      virtual reverse_iterator rend() { return _accessor.rbegin() + index_rend; }
      virtual const_reverse_iterator crend() const { return _accessor.crbegin() + index_rend; }

      virtual void swap(std::vector<T> &x) {
        x.resize(_info.length);
        std::swap_ranges( begin(), end(), x.begin() );
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendBufferingRegisterAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
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
      RegisterPath _registerPathName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information. We hold a copy of the RegisterInfo, since it might contain register accessors
      /// which may not be owned by the backend
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
