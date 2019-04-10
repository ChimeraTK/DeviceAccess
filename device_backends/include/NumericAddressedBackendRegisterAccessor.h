/*
 * NumericAddressedBackendRegisterAccessor.h
 *
 *  Created on: Mar 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H
#define CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H

#include "FixedPointConverter.h"
#include "ForwardDeclarations.h"
#include "IEEE754_SingleConverter.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "SyncNDRegisterAccessor.h"

namespace ChimeraTK {

  namespace detail {
    /** This function is external to allow template specialisation. */
    template<typename ConverterT>
    ConverterT createDataConverter(boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo);

    template<>
    FixedPointConverter createDataConverter<FixedPointConverter>(
        boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo);

    template<>
    IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(
        boost::shared_ptr<RegisterInfoMap::RegisterInfo> registerInfo);

    /** We need partial template specialisations of some functions. However, in C++ this is only possible for full classes.
     *  Hence we introduce an implementor class which only holds the functions which we have to re-implement anyway.
     *  The functions are doPostRead, doPreWrite and doPostWrite.
     */
    template<typename UserType, typename DataConverterType, bool isRaw>
    struct NumericAddressedPrePostActionsImplementor {
      // we just do references to the objects we need (even the shared ptr). The accessor we use it in guarantees the consistency.
      std::vector<std::vector<UserType>>& _buffer_2D;
      boost::shared_ptr<NumericAddressedLowLevelTransferElement>& _rawAccessor;
      size_t& _startAddress;
      DataConverterType& _dataConverter;

      NumericAddressedPrePostActionsImplementor(std::vector<std::vector<UserType>>& buffer,
          boost::shared_ptr<NumericAddressedLowLevelTransferElement>& rawAccessor, size_t& startAddress,
          DataConverterType& dataConverter)
      : _buffer_2D(buffer), _rawAccessor(rawAccessor), _startAddress(startAddress), _dataConverter(dataConverter) {}

      void doPostRead();
      void doPreWrite();
      void doPostWrite() {}
    };

    // only int32_t as raw needs special treatment
    template<typename DataConverterType>
    struct NumericAddressedPrePostActionsImplementor<int32_t, DataConverterType, true> {
      // we just do references to the objects we need (even the shared ptr). The accessor we use it in guarantees the consistency.
      std::vector<std::vector<int32_t>>& _buffer_2D;
      boost::shared_ptr<NumericAddressedLowLevelTransferElement>& _rawAccessor;
      size_t& _startAddress;

      NumericAddressedPrePostActionsImplementor(std::vector<std::vector<int32_t>>& buffer,
          boost::shared_ptr<NumericAddressedLowLevelTransferElement>& rawAccessor, size_t& startAddress,
          DataConverterType&)
      : _buffer_2D(buffer), _rawAccessor(rawAccessor), _startAddress(startAddress) {}

      void doPostRead();
      void doPreWrite();
      void doPostWrite();
    };

    template<typename UserType, typename DataConverterType, bool isRaw>
    void NumericAddressedPrePostActionsImplementor<UserType, DataConverterType, isRaw>::doPostRead() {
      auto itsrc = _rawAccessor->begin(_startAddress);
      _dataConverter.template vectorToCooked<UserType>(itsrc, itsrc + _buffer_2D[0].size(), _buffer_2D[0].begin());
    }

    template<typename UserType, typename DataConverterType, bool isRaw>
    void NumericAddressedPrePostActionsImplementor<UserType, DataConverterType, isRaw>::doPreWrite() {
      auto itsrc = _rawAccessor->begin(_startAddress);
      for(auto itdst = _buffer_2D[0].begin(); itdst != _buffer_2D[0].end(); ++itdst) {
        *itsrc = _dataConverter.template toRaw<UserType>(*itdst);
        ++itsrc;
      }
    }

    // special implementations for int32 raw
    template<typename DataConverterType>
    void NumericAddressedPrePostActionsImplementor<int32_t, DataConverterType, true>::doPostRead() {
      if(!_rawAccessor->isShared) {
        _buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
      else {
        auto itsrc = _rawAccessor->begin(_startAddress);
        auto itdst = _buffer_2D[0].begin();
        memcpy(&(*itdst), &(*itsrc), _buffer_2D[0].size() * sizeof(int32_t));
      }
    }

    template<typename DataConverterType>
    void NumericAddressedPrePostActionsImplementor<int32_t, DataConverterType, true>::doPreWrite() {
      if(!_rawAccessor->isShared) {
        _buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
      else {
        auto itdst = _rawAccessor->begin(_startAddress);
        auto itsrc = _buffer_2D[0].begin();
        memcpy(&(*itdst), &(*itsrc), _buffer_2D[0].size() * sizeof(int32_t));
      }
    }

    template<typename DataConverterType>
    void NumericAddressedPrePostActionsImplementor<int32_t, DataConverterType, true>::doPostWrite() {
      if(!_rawAccessor->isShared) {
        _buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
    }

  } // namespace detail

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for
   * scalar and 1D registers.
   */
  template<typename UserType, typename DataConverterType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor : public SyncNDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : SyncNDRegisterAccessor<UserType>(registerPathName), _dataConverter(registerPathName),
      _registerPathName(registerPathName), _numberOfWords(numberOfWords),
      _prePostActionsImplementor(NDRegisterAccessor<UserType>::buffer_2D, _rawAccessor, _startAddress, _dataConverter) {
      try {
        // check for unknown flags
        flags.checkForUnknownFlags({AccessMode::raw});

        // check device backend
        _dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev);
        if(!_dev) {
          throw ChimeraTK::logic_error("NumericAddressedBackendRegisterAccessor "
                                       "is used with a backend which is not "
                                       "a NumericAddressedBackend.");
        }

        // obtain register information
        boost::shared_ptr<RegisterInfo> info = _dev->getRegisterInfo(registerPathName);
        _registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
        _bar = _registerInfo->bar;
        _startAddress = _registerInfo->address + wordOffsetInRegister * sizeof(int32_t);

        // check number of words
        if(_numberOfWords == 0) {
          _numberOfWords = _registerInfo->getNumberOfElements() - wordOffsetInRegister;
        }
        if(_numberOfWords + wordOffsetInRegister > _registerInfo->getNumberOfElements()) {
          throw ChimeraTK::logic_error(
              "Requested number of words exceeds the size of the register '" + _registerPathName + "'!");
        }
        if(wordOffsetInRegister >= _registerInfo->getNumberOfElements()) {
          throw ChimeraTK::logic_error("Requested offset exceeds the size of the register'" + _registerPathName + "'!");
        }

        // create low-level transfer element handling the actual data transfer to
        // the hardware with raw data
        _rawAccessor.reset(new NumericAddressedLowLevelTransferElement(_dev, _bar, _startAddress, _numberOfWords));

        // allocated the buffers
        NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(_numberOfWords);

        // We don't have to fill it in a special way if the accessor is raw
        // because we have an overloaded, more efficient implementation
        // in this case. So we can use it in setAsCooked() and getAsCooked()
        _dataConverter = detail::createDataConverter<DataConverterType>(_registerInfo);

        if(flags.has(AccessMode::raw)) {
          if(typeid(UserType) != typeid(int32_t)) {
            throw ChimeraTK::logic_error("Given UserType when obtaining the "
                                         "NumericAddressedBackendRegisterAccessor in raw mode does not "
                                         "match the expected type. Use an int32_t instead! (Register "
                                         "name: " +
                _registerPathName + "')");
          }
          // FIXME: this has to move to the creation
          //isRaw = true;
        }
      }
      catch(...) {
        this->shutdown();
        throw;
      }

      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    virtual ~NumericAddressedBackendRegisterAccessor() { this->shutdown(); }

    void doReadTransfer() override { _rawAccessor->read(); }

    bool doReadTransferNonBlocking() override {
      _rawAccessor->read();
      return true;
    }

    bool doReadTransferLatest() override {
      _rawAccessor->read();
      return true;
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/ = {}) override {
      if(TransferElement::isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on an accessor which is part of a "
                                     "TransferGroup is not allowed "
                                     "(Register name: " +
            _registerPathName + "')");
      }
      _rawAccessor->write();
      return false;
    }

    void doPostRead() override {
      _prePostActionsImplementor.doPostRead();
      SyncNDRegisterAccessor<UserType>::doPostRead();
    }

    void doPreWrite() override { _prePostActionsImplementor.doPreWrite(); }

    void doPostWrite() override { _prePostActionsImplementor.doPostWrite(); }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<
          const NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>>(other);
      if(!rhsCasted) return false;
      if(_dev != rhsCasted->_dev) return false;
      if(_bar != rhsCasted->_bar) return false;
      if(_startAddress != rhsCasted->_startAddress) return false;
      if(_numberOfWords != rhsCasted->_numberOfWords) return false;
      // if(isRaw != rhsCasted->isRaw) return false; FIXME remove this line when cleaning up. obsolete with the template parameters. Just keeping in in case I have to revert
      if(_dataConverter != rhsCasted->_dataConverter) return false;
      return true;
    }

    bool isReadOnly() const override { return isReadable() && !isWriteable(); }

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

    // a local typename so the DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER does
    // not get confused by the comma which separates the two template parameters
    typedef NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw> THIS_TYPE;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, setAsCooked_impl, 3);

    AccessModeFlags getAccessModeFlags() const override {
      if(isRaw) return {AccessMode::raw};
      return {};
    }

    VersionNumber getVersionNumber() const override { return _rawAccessor->getVersionNumber(); }

   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    boost::shared_ptr<RegisterInfoMap::RegisterInfo> _registerInfo;

    /** Converter to interpret the data */
    DataConverterType _dataConverter;

    /** register and module name */
    RegisterPath _registerPathName;

    /** start address w.r.t. the PCIe bar */
    size_t _bar;

    /** start address w.r.t. the PCIe bar */
    size_t _startAddress;

    /** number of 4-byte words to access */
    size_t _numberOfWords;

    detail::NumericAddressedPrePostActionsImplementor<UserType, DataConverterType, isRaw> _prePostActionsImplementor;

    /** raw accessor */
    boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<NumericAddressedBackend> _dev;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return _rawAccessor->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      return {_rawAccessor}; // the rawAccessor always returns an empty list
    }

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
      auto casted = boost::dynamic_pointer_cast<NumericAddressedLowLevelTransferElement>(newElement);
      if(casted && casted->isMergeable(_rawAccessor)) {
        size_t newStartAddress = std::min(casted->_startAddress, _rawAccessor->_startAddress);
        size_t newStopAddress = std::max(
            casted->_startAddress + casted->_numberOfBytes, _rawAccessor->_startAddress + _rawAccessor->_numberOfBytes);
        size_t newNumberOfWords = (newStopAddress - newStartAddress) / sizeof(int32_t);
        casted->changeAddress(newStartAddress, newNumberOfWords);
        _rawAccessor = casted;
      }
    }

    /** A helper class to implement template specialisation on certain functions.
     *  We can do a partial specialisation on this class which we cannot/don't
     * want to do for the whole accessor.
     */
    template<typename RawT, typename CookedT>
    struct dataConverterTemplateSpecialisationHelper {
      static CookedT vectorToCooked(DataConverterType&, const typename std::vector<RawT>::const_iterator&,
          const typename std::vector<RawT>::const_iterator&, const typename std::vector<CookedT>::iterator&) {
        throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
      }
      static RawT toRaw(DataConverterType&, CookedT&) {
        throw ChimeraTK::logic_error("Seting as cooked is only available for raw accessors!");
      }
    };
    template<typename CookedT>
    struct dataConverterTemplateSpecialisationHelper<int32_t, CookedT> {
      static void vectorToCooked(DataConverterType& dataConverter,
          const typename std::vector<int32_t>::const_iterator& start,
          const typename std::vector<int32_t>::const_iterator& end,
          const typename std::vector<CookedT>::iterator& cooked) {
        dataConverter.template vectorToCooked<CookedT>(start, end, cooked);
      }
      static int32_t toRaw(DataConverterType& dataConverter, CookedT& value) { return dataConverter.toRaw(value); }
    };
  };

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType, typename DataConverterType, bool isRaw>
  template<typename COOKED_TYPE>
  COOKED_TYPE NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>::getAsCooked_impl(
      unsigned int channel, unsigned int sample) {
    if(isRaw) {
      std::vector<COOKED_TYPE> cookedData(1);
      dataConverterTemplateSpecialisationHelper<UserType, COOKED_TYPE>::vectorToCooked(_dataConverter,
          NDRegisterAccessor<UserType>::buffer_2D[channel].begin() + sample,
          NDRegisterAccessor<UserType>::buffer_2D[channel].begin() + sample + 1, cookedData.begin());
      return cookedData[0];
    }
    else {
      throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename UserType, typename DataConverterType, bool isRaw>
  template<typename COOKED_TYPE>
  void NumericAddressedBackendRegisterAccessor<UserType, DataConverterType, isRaw>::setAsCooked_impl(
      unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    if(isRaw) {
      NDRegisterAccessor<UserType>::buffer_2D[channel][sample] =
          dataConverterTemplateSpecialisationHelper<UserType, COOKED_TYPE>::toRaw(_dataConverter, value);
    }
    else {
      throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////

  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, FixedPointConverter, true);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, FixedPointConverter, false);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, true);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(
      NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter, false);

} // namespace ChimeraTK

#endif /* CHIMERA_TK_NUMERIC_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H */
