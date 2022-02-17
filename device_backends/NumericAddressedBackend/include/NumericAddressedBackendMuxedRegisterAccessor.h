#pragma once

#include <boost/shared_ptr.hpp>
#include <sstream>

#include "Exception.h"
#include "FixedPointConverter.h"
#include "MapFileParser.h"
#include "NumericAddressedBackend.h"
#include "RegisterInfoMap.h"
#include "NDRegisterAccessor.h"

namespace ChimeraTK {

  static const std::string MULTIPLEXED_SEQUENCE_PREFIX = "AREA_MULTIPLEXED_SEQUENCE_";
  static const std::string SEQUENCE_PREFIX = "SEQUENCE_";

  /********************************************************************************************************************/

  namespace detail {

    /** Iteration on a raw buffer with a given pitch (increment from one element to the next) in bytes */
    template<typename DATA_TYPE>
    struct pitched_iterator : std::iterator<std::random_access_iterator_tag, DATA_TYPE> {
      pitched_iterator(void* begin, size_t pitch) : _ptr(reinterpret_cast<uint8_t*>(begin)), _pitch(pitch) {}
      pitched_iterator& operator++() {
        _ptr += _pitch;
        return *this;
      }
      pitched_iterator operator++(int) {
        pitched_iterator retval = *this;
        ++(*this);
        return retval;
      }
      pitched_iterator operator+(size_t n) { return pitched_iterator(_ptr + n * _pitch, _pitch); }
      bool operator==(pitched_iterator other) const { return _ptr == other._ptr; }
      bool operator!=(pitched_iterator other) const { return !(*this == other); }
      size_t operator-(pitched_iterator other) const { return _ptr - other._ptr; }
      DATA_TYPE& operator*() const { return *reinterpret_cast<DATA_TYPE*>(_ptr); }

     private:
      uint8_t* _ptr;
      const size_t _pitch;
    };

  } // namespace detail

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for
   * multiplexd 2D registers
   */
  template<class UserType>
  class NumericAddressedBackendMuxedRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendMuxedRegisterAccessor(const RegisterPath& registerPathName, size_t numberOfElements,
        size_t elementsOffset, boost::shared_ptr<DeviceBackend> _backend);

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    void doPreRead(TransferType) override {
      if(!_ioDevice->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedBackendMuxedRegisterAccessor<UserType>>(other);
      if(!rhsCasted) return false;
      if(_ioDevice != rhsCasted->_ioDevice) return false;
      if(_bar != rhsCasted->_bar) return false;
      if(_address != rhsCasted->_address) return false;
      if(_nBytes != rhsCasted->_nBytes) return false;
      if(_numberOfElements != rhsCasted->_numberOfElements) return false;
      if(_elementsOffset != rhsCasted->_elementsOffset) return false;
      if(_converters != rhsCasted->_converters) return false;
      return true;
    }

    bool isReadOnly() const override { return isReadable() && !isWriteable(); }

    bool isReadable() const override { return _areaInfo->isReadable(); }

    bool isWriteable() const override { return !_isNotWriteable; }

    void makeWriteable() { _isNotWriteable = false; }

   protected:
    /** One fixed point converter for each sequence. */
    std::vector<FixedPointConverter> _converters;

    /** The device from (/to) which to perform the DMA transfer */
    boost::shared_ptr<NumericAddressedBackend> _ioDevice;

    /** number of data blocks / samples */
    size_t _nBlocks;

    std::vector<int32_t> _ioBuffer;

    boost::shared_ptr<NumericAddressedRegisterInfo> _areaInfo;
    std::vector<boost::shared_ptr<NumericAddressedRegisterInfo>> _sequenceInfos;

    std::vector<detail::pitched_iterator<int32_t>> _startIterators;
    std::vector<detail::pitched_iterator<int32_t>> _endIterators;

    uint32_t bytesPerBlock;

    /// register and module name
    std::string _moduleName, _registerName;
    RegisterPath _registerPathName;

    /// register address (after restricting to area of interes)
    size_t _bar, _address, _nBytes;

    /// area of interest
    size_t _numberOfElements;
    size_t _elementsOffset;

    /** cache for negated writeable status to avoid repeated evaluation */
    bool _isNotWriteable;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    using NDRegisterAccessor<UserType>::buffer_2D;
    using TransferElement::_exceptionBackend;
  };

  /********************************************************************************************************************/

  template<class UserType>
  NumericAddressedBackendMuxedRegisterAccessor<UserType>::NumericAddressedBackendMuxedRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfElements, size_t elementsOffset,
      boost::shared_ptr<DeviceBackend> _backend)
  : NDRegisterAccessor<UserType>(registerPathName, {}),
    _ioDevice(boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend)), _registerPathName(registerPathName),
    _numberOfElements(numberOfElements), _elementsOffset(elementsOffset) {
    // re-split register and module after merging names by the last dot (to
    // allow module.register in the register name)
    _registerPathName.setAltSeparator(".");
    auto moduleAndRegister = MapFileParser::splitStringAtLastDot(_registerPathName.getWithAltSeparator());
    _moduleName = moduleAndRegister.first;
    _registerName = moduleAndRegister.second;

    // build name of area as written in the map file
    std::string areaName = MULTIPLEXED_SEQUENCE_PREFIX + _registerName;

    // Obtain information about the area
    _areaInfo = _ioDevice->getRegisterInfo(registerPathName);

    // Cache writeability
    _isNotWriteable = !_areaInfo->isWriteable();

    // Obtain information for each sequence (= channel) in the area:
    // Create a fixed point converter for each sequence and store the sequence
    // information in a vector
    size_t iSeq = 0;
    while(true) {
      // build name of the next sequence as written in the map file
      boost::shared_ptr<NumericAddressedRegisterInfo> sequenceInfo;
      std::stringstream sequenceNameStream;
      sequenceNameStream << _moduleName << "." << SEQUENCE_PREFIX << _registerName << "_" << iSeq++;

      // try to find sequence
      try {
        sequenceInfo = _ioDevice->getRegisterInfo(sequenceNameStream.str());
      }
      catch(ChimeraTK::logic_error&) {
        // no sequence found: we are done
        // FIXME do not control program flow with exceptions... Check register existence beforehand!
        break;
      }

      // check consistency
      if(sequenceInfo->nElements != 1) {
        throw ChimeraTK::logic_error("Sequence words must have exactly one element");
      }

      // store sequence info and fixed point converter
      if(sequenceInfo->width > sequenceInfo->nBytes * 8) {
        // FIXME do we really want to permit and correct such map file errors?
        sequenceInfo->width = sequenceInfo->nBytes * 8;
      }
      _sequenceInfos.push_back(sequenceInfo);
      _converters.push_back(FixedPointConverter(
          registerPathName, sequenceInfo->width, sequenceInfo->nFractionalBits, sequenceInfo->signedFlag));
    }

    // check if no sequences were found
    if(_converters.empty()) {
      throw ChimeraTK::logic_error("No sequences found for name \"" + _registerName + "\".");
    }

    // compute size of one block in bytes (one sample for all channels)
    bytesPerBlock = 0;
    for(unsigned int i = 0; i < _converters.size(); i++) {
      uint32_t nbt = _sequenceInfos[i]->nBytes;
      bytesPerBlock += nbt;
      if(nbt != 1 && nbt != 2 && nbt != 4) {
        throw ChimeraTK::logic_error("Sequence word size must correspond to a primitive type");
      }
    }

    // compute number of blocks (number of samples for each channel)
    _nBlocks = std::floor(_areaInfo->nBytes / bytesPerBlock);

    // check number of words
    if(_elementsOffset >= _nBlocks) {
      throw ChimeraTK::logic_error("Requested element offset exceeds the size of the register!");
    }
    if(_numberOfElements == 0) {
      _numberOfElements = _nBlocks;
    }
    if(_numberOfElements + _elementsOffset > _nBlocks) {
      throw ChimeraTK::logic_error("Requested number of elements exceeds the size of the register! Requested end: " +
          std::to_string(_numberOfElements + _elementsOffset) + ", register length: " + std::to_string(_nBlocks));
    }

    // compute the address taking into account the selected area of interest
    _bar = _areaInfo->bar;
    _address = _areaInfo->address + bytesPerBlock * _elementsOffset;
    _nBytes = bytesPerBlock * _numberOfElements;
    if(_nBytes % sizeof(int32_t) > 0) _nBytes += 4 - _nBytes % sizeof(int32_t); // round up to the next multiple of 4
    if(_nBytes > _areaInfo->nBytes) {
      throw ChimeraTK::logic_error(
          "Requested number of elements exceeds the size of the register (late, redundant safety check)!");
    }
    _nBlocks = _numberOfElements;

    // allocate the buffer for the converted data
    NDRegisterAccessor<UserType>::buffer_2D.resize(_converters.size());
    for(size_t i = 0; i < _converters.size(); ++i) {
      NDRegisterAccessor<UserType>::buffer_2D[i].resize(_nBlocks);
    }

    // allocate the raw io buffer. Make it one element larger to make sure we can access the last byte via int32_t*
    _ioBuffer.resize(_nBytes / sizeof(int32_t) + 1);

    // compute pitched iterators for accessing the channels
    uint8_t* standOfMyioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
    for(size_t sequenceIndex = 0; sequenceIndex < _converters.size(); ++sequenceIndex) {
      _startIterators.push_back(detail::pitched_iterator<int32_t>(standOfMyioBuffer, bytesPerBlock));
      _endIterators.push_back(_startIterators.back() + _nBlocks);
      standOfMyioBuffer += _sequenceInfos[sequenceIndex]->nBytes;
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doReadTransferSynchronously() {
    _ioDevice->read(_bar, _address, _ioBuffer.data(), _nBytes);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      for(size_t i = 0; i < _converters.size(); ++i) {
        _converters[i].template vectorToCooked<UserType>(_startIterators[i], _endIterators[i], buffer_2D[i].begin());
      }
      // it is acceptable to create the version number in post read because this accessor does not have
      // wait_for_new_data. It is basically synchronous.
      this->_versionNumber = {};
      this->_dataValidity =
          DataValidity::ok; // we just read good data. Set validity back to ok if someone marked it faulty for writing.
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  bool NumericAddressedBackendMuxedRegisterAccessor<UserType>::doWriteTransfer(VersionNumber) {
    _ioDevice->write(_bar, _address, &(_ioBuffer[0]), _nBytes);
    return false;
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPreWrite(TransferType, VersionNumber) {
    if(!_ioDevice->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    uint8_t* standOfMyioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
    for(size_t blockIndex = 0; blockIndex < _nBlocks; ++blockIndex) {
      for(size_t sequenceIndex = 0; sequenceIndex < _converters.size(); ++sequenceIndex) {
        switch(_sequenceInfos[sequenceIndex]->nBytes) {
          case 1: // 8 bit variables
            *(standOfMyioBuffer) =
                _converters[sequenceIndex].toRaw(NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex]);
            standOfMyioBuffer++;
            break;
          case 2: // 16 bit variables
            *(reinterpret_cast<uint16_t*>(standOfMyioBuffer)) =
                _converters[sequenceIndex].toRaw(NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex]);
            standOfMyioBuffer = standOfMyioBuffer + 2;
            break;
          case 4: // 32 bit variables
            *(reinterpret_cast<uint32_t*>(standOfMyioBuffer)) =
                _converters[sequenceIndex].toRaw(NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex]);
            standOfMyioBuffer = standOfMyioBuffer + 4;
            break;
        }
      }
    }
  }
  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendMuxedRegisterAccessor);

} // namespace ChimeraTK
