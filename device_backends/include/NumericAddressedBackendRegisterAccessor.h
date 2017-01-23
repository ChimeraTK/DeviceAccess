/*
 * NumericAddressedBackendRegisterAccessor.h
 *
 *  Created on: Mar 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H
#define MTCA4U_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H

#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"
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
      : NDRegisterAccessor<UserType>(registerPathName),
        isRaw(false),
        _registerPathName(registerPathName),
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
          throw DeviceException("Requested number of words exceeds the size of the register!",
              DeviceException::WRONG_PARAMETER);
        }
        if(wordOffsetInRegister >= _registerInfo->getNumberOfElements()) {
          throw DeviceException("Requested offset exceeds the size of the register!",
              DeviceException::WRONG_PARAMETER);
        }

        // create low-level transfer element handling the actual data transfer to the hardware with raw data
        _rawAccessor.reset(new NumericAddressedLowLevelTransferElement(_dev,_bar,_startAddress,_numberOfWords));

        // allocated the buffers
        NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(_numberOfWords);

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
          isRaw = true;
        }
      }

      virtual ~NumericAddressedBackendRegisterAccessor() {};

      void doReadTransfer() override {
        _rawAccessor->read();
      }

      bool doReadTransferNonBlocking() override {
        _rawAccessor->read();
        return true;
      }

      void write() override {
        if(TransferElement::isInTransferGroup) {
          throw DeviceException("Calling read() or write() on an accessor which is part of a TransferGroup is not allowed.",
              DeviceException::NOT_IMPLEMENTED);
        }
        preWrite();
        _rawAccessor->write();
      }

      void postRead() override {
        auto itsrc = _rawAccessor->begin(_startAddress);
        for(auto itdst = NDRegisterAccessor<UserType>::buffer_2D[0].begin();
                 itdst != NDRegisterAccessor<UserType>::buffer_2D[0].end();
               ++itdst) {
          *itdst = _fixedPointConverter.toCooked<UserType>(*itsrc);
          ++itsrc;
        }
      };

      void preWrite() override {
        auto itsrc = _rawAccessor->begin(_startAddress);
        for(auto itdst = NDRegisterAccessor<UserType>::buffer_2D[0].begin();
                 itdst != NDRegisterAccessor<UserType>::buffer_2D[0].end();
               ++itdst) {
          *itsrc = _fixedPointConverter.toRaw<UserType>(*itdst);
          ++itsrc;
        }
      };

      void postWrite() override {
      };

      bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const override {
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedBackendRegisterAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(_bar != rhsCasted->_bar) return false;
        if(_startAddress != rhsCasted->_startAddress) return false;
        if(_numberOfWords != rhsCasted->_numberOfWords) return false;
        return true;
      }

      bool isReadOnly() const override {
        return false;
      }

      bool isReadable() const override {
        return true;
      }

      bool isWriteable() const override {
        return true;
      }

      FixedPointConverter getFixedPointConverter() const override {
        return _fixedPointConverter;
      }

    protected:

      /** Address, size and fixed-point representation information of the register from the map file */
      boost::shared_ptr<RegisterInfoMap::RegisterInfo> _registerInfo;

      /** Fixed point converter to interpret the data */
      FixedPointConverter _fixedPointConverter;
      bool isRaw;

      /** register and module name */
      RegisterPath _registerPathName;

      /** start address w.r.t. the PCIe bar */
      size_t _bar;

      /** start address w.r.t. the PCIe bar */
      size_t _startAddress;

      /** number of 4-byte words to access */
      size_t _numberOfWords;

      /** raw accessor */
      boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor;

      /** the backend to use for the actual hardware access */
      boost::shared_ptr<NumericAddressedBackend> _dev;

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
        return _rawAccessor->getHardwareAccessingElements();
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        auto casted = boost::dynamic_pointer_cast< NumericAddressedLowLevelTransferElement >(newElement);
        if(newElement->isSameRegister(_rawAccessor) && casted) {
          size_t newStartAddress = std::min(casted->_startAddress, _rawAccessor->_startAddress);
          size_t newStopAddress = std::max(casted->_startAddress+casted->_numberOfBytes,
                                           _rawAccessor->_startAddress+_rawAccessor->_numberOfBytes);
          size_t newNumberOfWords = (newStopAddress-newStartAddress)/sizeof(int32_t);
          casted->changeAddress(newStartAddress,newNumberOfWords);
          _rawAccessor = casted;
        }
      }

  };

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::postRead();

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::preWrite();

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::postWrite();

}    // namespace mtca4u

#endif /* MTCA4U_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H */
