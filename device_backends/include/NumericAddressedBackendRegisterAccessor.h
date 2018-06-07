/*
 * NumericAddressedBackendRegisterAccessor.h
 *
 *  Created on: Mar 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H
#define CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H

#include "SyncNDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "FixedPointConverter.h"
#include "ForwardDeclarations.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for scalar and 1D registers.
   */
  template<typename UserType>
  class NumericAddressedBackendRegisterAccessor : public SyncNDRegisterAccessor<UserType> {
    public:

      NumericAddressedBackendRegisterAccessor(boost::shared_ptr<DeviceBackend> dev,
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
      : SyncNDRegisterAccessor<UserType>(registerPathName),
        _fixedPointConverter(registerPathName),
        isRaw(false),
        _registerPathName(registerPathName),
        _numberOfWords(numberOfWords)
      {
        try {
          // check for unknown flags
          flags.checkForUnknownFlags({AccessMode::raw});

          // check device backend
          _dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev);
          if(!_dev) {
            throw DeviceException("NumericAddressedBackendRegisterAccessor is used with a backend which is not "
                "a NumericAddressedBackend.", DeviceException::WRONG_PARAMETER);
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
            throw DeviceException("Requested number of words exceeds the size of the register '"+_registerPathName+"'!",
                DeviceException::WRONG_PARAMETER);
          }
          if(wordOffsetInRegister >= _registerInfo->getNumberOfElements()) {
            throw DeviceException("Requested offset exceeds the size of the register'"+_registerPathName+"'!",
                DeviceException::WRONG_PARAMETER);
          }

          // create low-level transfer element handling the actual data transfer to the hardware with raw data
          _rawAccessor.reset(new NumericAddressedLowLevelTransferElement(_dev,_bar,_startAddress,_numberOfWords));

          // allocated the buffers
          NDRegisterAccessor<UserType>::buffer_2D.resize(1);
          NDRegisterAccessor<UserType>::buffer_2D[0].resize(_numberOfWords);

          // configure fixed point converter
          // We don't have to fill it in a special way if the accessor is raw
          // because we have an overloaded, more efficient implementation
          // in this case. So we can use it in setAsCoocked() and getAsCoocked()
          _fixedPointConverter = FixedPointConverter(_registerPathName,
                                                     _registerInfo->width,
                                                     _registerInfo->nFractionalBits,
                                                     _registerInfo->signedFlag);
          if(flags.has(AccessMode::raw)) {
            if(typeid(UserType) != typeid(int32_t)) {
              throw DeviceException("Given UserType when obtaining the NumericAddressedBackendRegisterAccessor in raw mode does not "
                  "match the expected type. Use an int32_t instead! (Register name: "+_registerPathName+"')", DeviceException::WRONG_PARAMETER);
            }
            isRaw = true;
          }
        }
        catch(...) {
          this->shutdown();
          throw;
        }

        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCoocked_impl);
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCoocked_impl);
      }

      virtual ~NumericAddressedBackendRegisterAccessor() {
        this->shutdown();
      }

      void doReadTransfer() override {
        _rawAccessor->read();
      }

      bool doReadTransferNonBlocking() override {
        _rawAccessor->read();
        return true;
      }

      bool doReadTransferLatest() override {
        _rawAccessor->read();
        return true;
      }

      bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/={}) override {
        if(TransferElement::isInTransferGroup) {
          throw DeviceException("Calling read() or write() on an accessor which is part of a TransferGroup is not allowed "
                                "(Register name: "+_registerPathName+"')", DeviceException::NOT_IMPLEMENTED);
        }
        _rawAccessor->write();
        return false;
      }

      void doPostRead() override {
        auto itsrc = _rawAccessor->begin(_startAddress);
        for(auto itdst = NDRegisterAccessor<UserType>::buffer_2D[0].begin();
                 itdst != NDRegisterAccessor<UserType>::buffer_2D[0].end();
               ++itdst) {
          *itdst = _fixedPointConverter.toCooked<UserType>(*itsrc);
          ++itsrc;
        }
        SyncNDRegisterAccessor<UserType>::doPostRead();
      }

      void doPreWrite() override {
        auto itsrc = _rawAccessor->begin(_startAddress);
        for(auto itdst = NDRegisterAccessor<UserType>::buffer_2D[0].begin();
                 itdst != NDRegisterAccessor<UserType>::buffer_2D[0].end();
               ++itdst) {
          *itsrc = _fixedPointConverter.toRaw<UserType>(*itdst);
          ++itsrc;
        }
      }

      void doPostWrite() override {
      }

      bool mayReplaceOther(const boost::shared_ptr<TransferElement const> &other) const override {
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedBackendRegisterAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(_bar != rhsCasted->_bar) return false;
        if(_startAddress != rhsCasted->_startAddress) return false;
        if(_numberOfWords != rhsCasted->_numberOfWords) return false;
        if(isRaw != rhsCasted->isRaw) return false;
        if(_fixedPointConverter != rhsCasted->_fixedPointConverter) return false;
        return true;
      }

      bool isReadOnly() const override {
        return isReadable() && !isWriteable();
      }

      bool isReadable() const override {
        return (_registerInfo->registerAccess & RegisterInfoMap::RegisterInfo::Access::READ) != 0;
      }

      bool isWriteable() const override {
        return (_registerInfo->registerAccess & RegisterInfoMap::RegisterInfo::Access::WRITE) != 0;
      }

      /** Get the FixedPointConverter. In case of a raw accessor this is the
       *  conversion that would be used if the data should be coocked.
       */
      FixedPointConverter getFixedPointConverter() const override {
        return _fixedPointConverter;
      }

      template<typename COOCKED_TYPE>
      COOCKED_TYPE getAsCoocked_impl(unsigned int channel, unsigned int sample);

      template<typename COOCKED_TYPE>
      void setAsCoocked_impl(unsigned int channel, unsigned int sample, COOCKED_TYPE value);

      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( NumericAddressedBackendRegisterAccessor<UserType>, getAsCoocked_impl, 2 );
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( NumericAddressedBackendRegisterAccessor<UserType>, setAsCoocked_impl, 3 );

      AccessModeFlags getAccessModeFlags() const override {
        if(isRaw) return { AccessMode::raw };
        return {};
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

      std::list< boost::shared_ptr<TransferElement> > getInternalElements() override {
        return {_rawAccessor};    // the rawAccessor always returns an empty list
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        auto casted = boost::dynamic_pointer_cast< NumericAddressedLowLevelTransferElement >(newElement);
        if(casted && casted->isMergeable(_rawAccessor)) {
          size_t newStartAddress = std::min(casted->_startAddress, _rawAccessor->_startAddress);
          size_t newStopAddress = std::max(casted->_startAddress+casted->_numberOfBytes,
                                           _rawAccessor->_startAddress+_rawAccessor->_numberOfBytes);
          size_t newNumberOfWords = (newStopAddress-newStartAddress)/sizeof(int32_t);
          casted->changeAddress(newStartAddress,newNumberOfWords);
          _rawAccessor = casted;
        }
      }

  };

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType> template<typename COOCKED_TYPE>
  COOCKED_TYPE NumericAddressedBackendRegisterAccessor<UserType>::getAsCoocked_impl(unsigned int /*channel*/, unsigned int /*sample*/){
    // This is a coocked accessor. For the only possible raw type (int32_t) we have a
    // template specialisation (instead of a throwing one for strings).
    throw DeviceException("Getting as coocked is only available for raw accessors!", DeviceException::NOT_AVAILABLE);
  }
  template<> template<typename COOCKED_TYPE>
  COOCKED_TYPE NumericAddressedBackendRegisterAccessor<int32_t>::getAsCoocked_impl(unsigned int channel, unsigned int sample){
    if(isRaw){
      return _fixedPointConverter.toCooked<COOCKED_TYPE>(NDRegisterAccessor<int32_t>::buffer_2D[channel][sample]);
    }else{
      throw DeviceException("Getting as coocked is only available for raw accessors!", DeviceException::NOT_AVAILABLE);
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType> template<typename COOCKED_TYPE>
  void NumericAddressedBackendRegisterAccessor<UserType>::setAsCoocked_impl(unsigned int /*channel*/, unsigned int /*sample*/, COOCKED_TYPE /*value*/){
    // This is a coocked accessor. For the only possible raw type (int32_t) we have a
    // template specialisation (instead of a throwing one for strings).
    throw DeviceException("Setting as coocked is only available for raw accessors!", DeviceException::NOT_AVAILABLE);
  }
  template<> template<typename COOCKED_TYPE>
    void NumericAddressedBackendRegisterAccessor<int32_t>::setAsCoocked_impl(unsigned int channel, unsigned int sample, COOCKED_TYPE value){
    if(isRaw){
      NDRegisterAccessor<int32_t>::buffer_2D[channel][sample] = _fixedPointConverter.toRaw(value);
    }else{
      throw DeviceException("Setting as coocked is only available for raw accessors!", DeviceException::NOT_AVAILABLE);
    }
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::doPostRead();

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::doPreWrite();

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::doPostWrite();

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor);


}    // namespace ChimeraTK

#endif /* CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H */
