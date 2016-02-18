/*
 * LNMBackendBufferingVariableAccessor.h - Access a variable or constant in a logical name mapping file with a
 *                                         buffering-type accessor. There is obviously no actual buffering required,
 *                                         though, yet we provide the same interface as for any other register.
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

      LNMBackendBufferingVariableAccessor(boost::shared_ptr<DeviceBackend> dev, const std::string &module,
          const std::string &registerName)
      : _registerName(registerName),_moduleName(module)
      {
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        std::string name = ( module.length() > 0 ? module + "." + registerName : registerName );
        _info = _dev->_map.getRegisterInfo(name);
        if( _info.targetType != LogicalNameMap::TargetType::INT_CONSTANT ) {
          throw DeviceException("LNMBackendBufferingConstantAccessor used for wrong register type.",
              DeviceException::EX_WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
        }
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(1);
        BufferingRegisterAccessorImpl<T>::cookedBuffer[0] = _info.value;
      }

      virtual ~LNMBackendBufferingVariableAccessor() {};

      virtual void read() {
        // no need to read anything
      }

      virtual void write() {
        throw DeviceException("Writing to constant-type registers of logical name mapping devices is not possible.",
            DeviceException::REGISTER_IS_READ_ONLY);
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
        if(_registerName != rhsCasted->_registerName) return false;
        if(_moduleName != rhsCasted->_moduleName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return true;
      }

    protected:

      /// register and module name
      std::string _registerName, _moduleName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information
      LogicalNameMap::RegisterInfo _info;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) {}  // LCOV_EXCL_LINE

  };

}    // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H */
