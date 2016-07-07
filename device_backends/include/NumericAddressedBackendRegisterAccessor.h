/*
 * MemoryAddressedBackendBufferingRegisterAccessor.h
 *
 *  Created on: Mar 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_MEMORY_ADDRESSED_BACKEND_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_MEMORY_ADDRESSED_BACKEND_BUFFERING_REGISTER_ACCESSOR_H

#include "NDRegisterAccessor.h"
#include "NumericAddressedBackend.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for scalar and 1D registers.
   */
  template<typename UserType>
  class NumericAddressedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
    public:

      NumericAddressedBackendRegisterAccessor(boost::shared_ptr<DeviceBackend> dev,
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
      : _registerPathName(registerPathName),
        _numberOfWords(numberOfWords)
      {
        // check for unknown flags
        flags.checkForUnknownFlags({AccessMode::raw});
        // check device backend
        _dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev);
        if(!_dev) {
          throw DeviceException("MemoryAddressedBackendBufferingRegisterAccessor is used with a backend which is not "
              "a MemoryAddressedBackend.", DeviceException::WRONG_PARAMETER);
        }

        // obtain register information
        boost::shared_ptr<RegisterInfo> info = _dev->getRegisterInfo(registerPathName);
        _registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
        _bar = _registerInfo->bar;
        _startAddress = _registerInfo->address + wordOffsetInRegister*sizeof(int32_t);

        // check number of words
        if(_numberOfWords == 0) {
          _numberOfWords = _registerInfo->getNumberOfElements() - wordOffsetInRegister;
        }
        if(_numberOfWords + wordOffsetInRegister > _registerInfo->getNumberOfElements()) {
          throw DeviceException("Requested number of words exceed the size of the register!",
              DeviceException::WRONG_PARAMETER);
        }

        // allocated the buffers
        NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(_numberOfWords);
        rawDataBuffer.resize(_numberOfWords);

        // compute number of bytes
        _numberOfBytes = _numberOfWords*sizeof(int32_t);

        // configure fixed point converter
        if(!flags.has(AccessMode::raw)) {
          _fixedPointConverter = FixedPointConverter(_registerInfo->width, _registerInfo->nFractionalBits,
              _registerInfo->signedFlag);
        }
        else {
          if(typeid(UserType) != typeid(int32_t)) {
            throw DeviceException("Given UserType when obtaining the BufferingRegisterAccessor in raw mode does not "
                "match the expected type. Use an int32_t instead!", DeviceException::WRONG_PARAMETER);
          }
          _fixedPointConverter = FixedPointConverter(32, 0, true);
        }
      }

      virtual ~NumericAddressedBackendRegisterAccessor() {};

      virtual void read() {
        _dev->read(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
        for(size_t i=0; i < _numberOfWords; ++i){
          NDRegisterAccessor<UserType>::buffer_2D[0][i] = _fixedPointConverter.toCooked<UserType>(rawDataBuffer[i]);
        }
      }

      virtual void write() {
        for(size_t i=0; i < _numberOfWords; ++i){
          rawDataBuffer[i] = _fixedPointConverter.toRaw(NDRegisterAccessor<UserType>::buffer_2D[0][i]);
        }
        _dev->write(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedBackendRegisterAccessor<UserType> >(other);
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
      boost::shared_ptr<NumericAddressedBackend> _dev;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) {} // LCOV_EXCL_LINE

  };

}    // namespace mtca4u

#endif /* MTCA4U_MEMORY_ADDRESSED_BACKEND_BUFFERING_REGISTER_ACCESSOR_H */
