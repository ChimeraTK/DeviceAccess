/*
 * BackendBufferingRegisterAccessor.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BACKEND_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BACKEND_BUFFERING_REGISTER_ACCESSOR_H

#include "BufferingRegisterAccessorImpl.h"
#include "FixedPointConverter.h"
#include "RegisterAccessor.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/
  /** Standard implementation of the BufferingRegisterAccessor. This implementation uses internally a non-buffering
   *  register accessor to aquire the data. It should be suitable for most backends.
   */
  template<typename T>
  class BackendBufferingRegisterAccessor : public BufferingRegisterAccessorImpl<T> {
    public:

      BackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const std::string &module,
          const std::string &registerName)
      : _registerName(registerName),_moduleName(module),_dev(dev)
      {
        _accessor = dev->getRegisterAccessor(registerName, module);
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(_accessor->getNumberOfElements());
      }

      virtual ~BackendBufferingRegisterAccessor() {};

      virtual void read() {
        _accessor->read(BufferingRegisterAccessorImpl<T>::cookedBuffer.data(),
            BufferingRegisterAccessorImpl<T>::getNumberOfElements());
      }

      virtual void write() {
        _accessor->write(BufferingRegisterAccessorImpl<T>::cookedBuffer.data(),
            BufferingRegisterAccessorImpl<T>::getNumberOfElements());
      }

      virtual bool sameRegister(const TransferElement &rightHandSide) const {
        auto rhsCasted = dynamic_cast<const BackendBufferingRegisterAccessor<T>*>(&rightHandSide);
        if(!rhsCasted) return false;
        if(_registerName != rhsCasted->_registerName) return false;
        if(_moduleName != rhsCasted->_moduleName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

    protected:

      /// pointer to non-buffering accessor
      boost::shared_ptr< RegisterAccessor > _accessor;

      /// register and module name
      std::string _registerName, _moduleName;

      /// device
      boost::shared_ptr<DeviceBackend> _dev;

  };

}    // namespace mtca4u


#endif /* MTCA4U_BACKEND_BUFFERING_REGISTER_ACCESSOR_H */
