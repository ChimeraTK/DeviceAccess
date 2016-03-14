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

      BackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess)
      : _registerPathName(registerPathName),
        _numberOfWords(numberOfWords),
        _wordOffsetInRegister(wordOffsetInRegister),
        _dev(dev)
      {
        // obtain register accessor
        _accessor = dev->getRegisterAccessor(registerPathName, "");
        // check number of words
        if(_numberOfWords == 0) {
          _numberOfWords = _accessor->getNumberOfElements();
        }
        if(_numberOfWords + wordOffsetInRegister > _accessor->getNumberOfElements()) {
          throw DeviceException("Requested number of words exceed the size of the register!",
              DeviceException::EX_WRONG_PARAMETER);
        }
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(_numberOfWords);
        // switch to raw access if requested
        if(enforceRawAccess) {
          if(typeid(T) != typeid(int32_t)) {
            throw DeviceException("Given UserType when obtaining the BufferingRegisterAccessor in raw mode does not "
                "match the expected type. Use an int32_t instead!", DeviceException::EX_WRONG_PARAMETER);
          }
          _accessor->getFixedPointConverter().reconfigure(32,0,true);
        }
      }
/*
      BackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const std::string &module,
          const std::string &registerName)
      : _registerName(registerName),_moduleName(module),_dev(dev)
      {
        _accessor = dev->getRegisterAccessor(registerName, module);
        BufferingRegisterAccessorImpl<T>::cookedBuffer.resize(_accessor->getNumberOfElements());
      }
*/
      virtual ~BackendBufferingRegisterAccessor() {};

      virtual void read() {
        _accessor->read(BufferingRegisterAccessorImpl<T>::cookedBuffer.data(),_numberOfWords,_wordOffsetInRegister);
      }

      virtual void write() {
        _accessor->write(BufferingRegisterAccessorImpl<T>::cookedBuffer.data(),_numberOfWords,_wordOffsetInRegister);
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const BackendBufferingRegisterAccessor<T> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return false;
      }

    protected:

      /// pointer to non-buffering accessor
      boost::shared_ptr< RegisterAccessor > _accessor;

      /// register and module name
      RegisterPath _registerPathName;

      /// number of words to access
      size_t _numberOfWords;

      /// offset in words into the register
      size_t _wordOffsetInRegister;

      /// device
      boost::shared_ptr<DeviceBackend> _dev;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) {} // LCOV_EXCL_LINE

  };

}    // namespace mtca4u


#endif /* MTCA4U_BACKEND_BUFFERING_REGISTER_ACCESSOR_H */
