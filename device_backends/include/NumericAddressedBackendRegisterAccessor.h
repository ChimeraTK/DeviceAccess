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
#include "IEEE754_SingleConverter.h"
#include "ForwardDeclarations.h"

namespace ChimeraTK {

  /** This function is external to allow template specialisation. */
  namespace detail{
    template<typename ConverterT>
    ConverterT createDataConverter(boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo);

    template<>
    FixedPointConverter createDataConverter<FixedPointConverter>(boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo);

    template<>
    IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo);

  }

  
  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for scalar and 1D registers.
   */
  template<typename UserType, typename DataConverterType>
  class NumericAddressedBackendRegisterAccessor : public SyncNDRegisterAccessor<UserType> {

    public:

      NumericAddressedBackendRegisterAccessor(boost::shared_ptr<DeviceBackend> dev,
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
      : SyncNDRegisterAccessor<UserType>(registerPathName),
        _dataConverter(registerPathName),
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
            throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor is used with a backend which is not "
                "a NumericAddressedBackend.");
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
            throw ChimeraTK::logic_error("Requested number of words exceeds the size of the register '"+_registerPathName+"'!");
          }
          if(wordOffsetInRegister >= _registerInfo->getNumberOfElements()) {
            throw ChimeraTK::logic_error("Requested offset exceeds the size of the register'"+_registerPathName+"'!");
          }

          // create low-level transfer element handling the actual data transfer to the hardware with raw data
          _rawAccessor.reset(new NumericAddressedLowLevelTransferElement(_dev,_bar,_startAddress,_numberOfWords));

          // allocated the buffers
          NDRegisterAccessor<UserType>::buffer_2D.resize(1);
          NDRegisterAccessor<UserType>::buffer_2D[0].resize(_numberOfWords);

          // We don't have to fill it in a special way if the accessor is raw
          // because we have an overloaded, more efficient implementation
          // in this case. So we can use it in setAsCooked() and getAsCooked()
          _dataConverter = detail::createDataConverter<DataConverterType>( _registerInfo );
          
          if(flags.has(AccessMode::raw)) {
            if(typeid(UserType) != typeid(int32_t)) {
              throw ChimeraTK::logic_error("Given UserType when obtaining the NumericAddressedBackendRegisterAccessor in raw mode does not "
                  "match the expected type. Use an int32_t instead! (Register name: "+_registerPathName+"')");
            }
            isRaw = true;
          }
        }
        catch(...) {
          this->shutdown();
          throw;
        }

        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
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
          throw ChimeraTK::logic_error("Calling read() or write() on an accessor which is part of a TransferGroup is not allowed "
                                "(Register name: "+_registerPathName+"')");
        }
        _rawAccessor->write();
        return false;
      }

      void doPostRead() override {
        auto itsrc = _rawAccessor->begin(_startAddress);
        for(auto itdst = NDRegisterAccessor<UserType>::buffer_2D[0].begin();
                 itdst != NDRegisterAccessor<UserType>::buffer_2D[0].end();
               ++itdst) {
          *itdst = _dataConverter.template toCooked<UserType>(*itsrc);
          ++itsrc;
        }
        SyncNDRegisterAccessor<UserType>::doPostRead();
      }

      void doPreWrite() override {
        auto itsrc = _rawAccessor->begin(_startAddress);
        for(auto itdst = NDRegisterAccessor<UserType>::buffer_2D[0].begin();
                 itdst != NDRegisterAccessor<UserType>::buffer_2D[0].end();
               ++itdst) {
          *itsrc = _dataConverter.template toRaw<UserType>(*itdst);
          ++itsrc;
        }
      }

      void doPostWrite() override {
      }

      bool mayReplaceOther(const boost::shared_ptr<TransferElement const> &other) const override {
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedBackendRegisterAccessor<UserType, DataConverterType> >(other);
        if(!rhsCasted) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(_bar != rhsCasted->_bar) return false;
        if(_startAddress != rhsCasted->_startAddress) return false;
        if(_numberOfWords != rhsCasted->_numberOfWords) return false;
        if(isRaw != rhsCasted->isRaw) return false;
        if(_dataConverter != rhsCasted->_dataConverter) return false;
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

      template<typename COOKED_TYPE>
      COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample);

      template<typename COOKED_TYPE>
      void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value);

      // a local typename so the DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER does not get confused
      // by the comma which separates the two template parameters
      typedef NumericAddressedBackendRegisterAccessor<UserType, DataConverterType> THIS_TYPE;
      
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( THIS_TYPE, getAsCooked_impl, 2 );
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( THIS_TYPE, setAsCooked_impl, 3 );

      AccessModeFlags getAccessModeFlags() const override {
        if(isRaw) return { AccessMode::raw };
        return {};
      }

    protected:

      /** Address, size and fixed-point representation information of the register from the map file */
      boost::shared_ptr<RegisterInfoMap::RegisterInfo> _registerInfo;

      /** Converter to interpret the data */
      DataConverterType _dataConverter;
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

      /** A helper class to implement template specialisation on certain functions.
       *  We can do a partial specialisation on this class which we cannot/don't want to do
       *  for the whole accessor.
       */
      template<typename RawT, typename CookedT>
      struct dataConverterTemplateSpecialisationHelper{
        static CookedT toCooked( DataConverterType &, RawT & ){
          throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
        }
        static RawT toRaw( DataConverterType &, CookedT & ){
          throw ChimeraTK::logic_error("Seting as cooked is only available for raw accessors!");
        }
      };
      template<typename CookedT>
      struct dataConverterTemplateSpecialisationHelper<int32_t, CookedT>{
        static CookedT toCooked( DataConverterType & dataConverter, int32_t & rawValue){
          return dataConverter.template toCooked<CookedT>(rawValue);
        }
        static int32_t toRaw( DataConverterType & dataConverter, CookedT & value){
          return dataConverter.toRaw(value);
        }
      };

      
  };

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType, typename DataConverterType> template<typename COOKED_TYPE>
  COOKED_TYPE NumericAddressedBackendRegisterAccessor<UserType, DataConverterType>::getAsCooked_impl(unsigned int channel, unsigned int sample){
    if(isRaw){
      return dataConverterTemplateSpecialisationHelper<UserType,COOKED_TYPE>::toCooked(_dataConverter, NDRegisterAccessor<UserType>::buffer_2D[channel][sample]);
    }else{
      throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType, typename DataConverterType> template<typename COOKED_TYPE>
  void NumericAddressedBackendRegisterAccessor<UserType, DataConverterType>::setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value){
    if(isRaw){
      NDRegisterAccessor<UserType>::buffer_2D[channel][sample] =
        dataConverterTemplateSpecialisationHelper<UserType,COOKED_TYPE>::toRaw(_dataConverter, value);
    }else{
      throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
    }
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t, FixedPointConverter>::doPostRead();

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t, FixedPointConverter>::doPreWrite();

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t, FixedPointConverter>::doPostWrite();

  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, FixedPointConverter);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter);


}    // namespace ChimeraTK

#endif /* CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H */
