/*
 * LNMBackendBufferingVariableAccessor.h - Access a variable or constant in a logical name mapping file with a
 *                                         buffering-type accessor.
 *
 *  Created on: Feb 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H
#define MTCA4U_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H

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
  class LNMBackendBufferingVariableAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      LNMBackendBufferingVariableAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess)
      : _registerPathName(registerPathName), _fixedPointConverter(32, 0, 1)
      {
        if(wordOffsetInRegister != 0 || numberOfWords > 1) {
          throw DeviceException("LNMBackendBufferingVariableAccessor: offset and number of words not "
              "supported!", DeviceException::NOT_IMPLEMENTED); // LCOV_EXCL_LINE (impossible to test...)
        }
        if(enforceRawAccess) {
          if(typeid(T) != typeid(int32_t)) {
            throw DeviceException("Given UserType when obtaining the LNMBackendBufferingVariableAccessor in raw mode"
                " does not match the expected type. Use an int32_t instead!", DeviceException::EX_WRONG_PARAMETER);
          }
        }
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // obtain the register info
        _info = boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(_registerPathName));
        // check for incorrect usage of this accessor
        if( _info->targetType != LogicalNameMap::TargetType::INT_CONSTANT &&
            _info->targetType != LogicalNameMap::TargetType::INT_VARIABLE    ) {
          throw DeviceException("LNMBackendBufferingVariableAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(1);
        BufferingRegisterAccessorImpl<T>::cookedBuffer[0] = _fixedPointConverter.toCooked<T>(_info->value);
      }

      virtual ~LNMBackendBufferingVariableAccessor() {};

      virtual void read() {
        BufferingRegisterAccessorImpl<T>::cookedBuffer[0] =_fixedPointConverter.toCooked<T>(_info->value);
      }

      virtual void write() {
        if(isReadOnly()) {
          throw DeviceException("Writing to constant-type registers of logical name mapping devices is not possible.",
              DeviceException::REGISTER_IS_READ_ONLY);
        }
        _info->value = _fixedPointConverter.toRaw(BufferingRegisterAccessorImpl<T>::cookedBuffer[0]);
      }

      virtual T& operator[](unsigned int) {
        return BufferingRegisterAccessorImpl<T>::cookedBuffer[0];
      }

      virtual unsigned int getNumberOfElements() {
        return 1;
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const LNMBackendBufferingVariableAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return _info->targetType == LogicalNameMap::TargetType::INT_CONSTANT;
      }

      virtual FixedPointConverter getFixedPointConverter() const {
        throw DeviceException("Not implemented", DeviceException::NOT_IMPLEMENTED);
      }

    protected:

      /// register and module name
      RegisterPath _registerPathName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information. We have a shared pointer to the original RegisterInfo inside the map, since
      /// we need to modify the value in it (in case of a writeable variable register)
      boost::shared_ptr<LogicalNameMap::RegisterInfo> _info;

      /// fixed point converter to handle type conversions from our "raw" type int to the requested user type.
      /// Note: no actual fixed point conversion is done, it is just used for the type conversion!
      FixedPointConverter _fixedPointConverter;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) {}  // LCOV_EXCL_LINE

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H */
