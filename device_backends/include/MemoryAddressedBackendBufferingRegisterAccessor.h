/*
 * MemoryAddressedBackendBufferingRegisterAccessor.h
 *
 *  Created on: Mar 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_MEMORY_ADDRESSED_BACKEND_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_MEMORY_ADDRESSED_BACKEND_BUFFERING_REGISTER_ACCESSOR_H

#include "BufferingRegisterAccessorImpl.h"
#include "MemoryAddressedBackend.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/
  /** Standard implementation of the BufferingRegisterAccessor. This implementation uses internally a non-buffering
   *  register accessor to aquire the data. It should be suitable for most backends.
   */
  template<typename UserType>
  class MemoryAddressedBackendBufferingRegisterAccessor : public BufferingRegisterAccessorImpl<UserType> {
    public:

      MemoryAddressedBackendBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev,
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess)
      : _registerPathName(registerPathName),
        _numberOfWords(numberOfWords)
      {
        // check device backend
        _dev = boost::dynamic_pointer_cast<MemoryAddressedBackend>(dev);
        if(!_dev) {
          throw DeviceException("MemoryAddressedBackendBufferingRegisterAccessor is used with a backend which is not "
              "a MemoryAddressedBackend.", DeviceException::EX_WRONG_PARAMETER);
        }

        // obtain register information
        const RegisterCatalogue& catalogue = dev->getRegisterCatalogue();
        boost::shared_ptr<RegisterInfo> info = catalogue.getRegister(registerPathName);
        _registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
        _bar = _registerInfo->bar;
        _startAddress = _registerInfo->address + wordOffsetInRegister*sizeof(int32_t);

        // check number of words
        if(_numberOfWords == 0) {
          _numberOfWords = _registerInfo->getNumberOfElements();
        }
        if(_numberOfWords + wordOffsetInRegister > _registerInfo->getNumberOfElements()) {
          throw DeviceException("Requested number of words ("+std::to_string(_numberOfWords)+" with an offset of "+
              std::to_string(wordOffsetInRegister)+") exceed the size of the register ("+
              std::to_string(_registerInfo->getNumberOfElements())+")!",
              DeviceException::EX_WRONG_PARAMETER);
        }

        // allocated the buffers
        BufferingRegisterAccessorImpl<UserType>::cookedBuffer.resize(_numberOfWords);
        rawDataBuffer.resize(_numberOfWords);

        // compute number of bytes
        _numberOfBytes = _numberOfWords*sizeof(int32_t);

        // configure fixed point converter
        if(!enforceRawAccess) {
          _fixedPointConverter = FixedPointConverter(_registerInfo->width, _registerInfo->nFractionalBits,
              _registerInfo->signedFlag);
        }
        else {
          if(typeid(UserType) != typeid(int32_t)) {
            throw DeviceException("Given UserType when obtaining the BufferingRegisterAccessor in raw mode does not "
                "match the expected type. Use an int32_t instead!", DeviceException::EX_WRONG_PARAMETER);
          }
          _fixedPointConverter = FixedPointConverter(32, 0, true);
        }
      }

      virtual ~MemoryAddressedBackendBufferingRegisterAccessor() {};

      virtual void read() {
        _dev->read(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
        for(size_t i=0; i < _numberOfWords; ++i){
          BufferingRegisterAccessorImpl<UserType>::cookedBuffer[i] = _fixedPointConverter.toCooked<UserType>(rawDataBuffer[i]);
        }
      }

      virtual void write() {
        for(size_t i=0; i < _numberOfWords; ++i){
          rawDataBuffer[i] = _fixedPointConverter.toRaw(BufferingRegisterAccessorImpl<UserType>::cookedBuffer[i]);
        }
        _dev->write(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const MemoryAddressedBackendBufferingRegisterAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(_bar != rhsCasted->_bar) return false;
        if(_startAddress != rhsCasted->_startAddress) return false;
        if(_numberOfWords != rhsCasted->_numberOfWords) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return false;
      }

      virtual FixedPointConverter getFixedPointConverter() const {
        return _fixedPointConverter;
      }

    protected:

      /** Address, size and fixed-point representation information of the register from the map file */
      boost::shared_ptr<RegisterInfoMap::RegisterInfo> _registerInfo;

      /** Fixed point converter to interpret the data */
      FixedPointConverter _fixedPointConverter;

      /** register and module name */
      RegisterPath _registerPathName;

      /** start address w.r.t. the PCIe bar */
      size_t _bar;

      /** start address w.r.t. the PCIe bar */
      size_t _startAddress;

      /** number of 4-byte words to access */
      size_t _numberOfWords;

      /** number of bytes to access */
      size_t _numberOfBytes;

      /** raw buffer */
      std::vector<int32_t> rawDataBuffer;

      /** the backend to use for the actual hardware access */
      boost::shared_ptr<MemoryAddressedBackend> _dev;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) {} // LCOV_EXCL_LINE

  };

}    // namespace mtca4u

#endif /* MTCA4U_MEMORY_ADDRESSED_BACKEND_BUFFERING_REGISTER_ACCESSOR_H */
