#ifndef MTCA4U_MEMORY_ADDRESSED_BACKEND_TWO_D_REGISTER_ACCESSOR_H
#define MTCA4U_MEMORY_ADDRESSED_BACKEND_TWO_D_REGISTER_ACCESSOR_H

#include <sstream>
#include <boost/shared_ptr.hpp>

#include "NDRegisterAccessor.h"
#include "RegisterInfoMap.h"
#include "FixedPointConverter.h"
#include "NumericAddressedBackend.h"
#include "Exception.h"
#include "MapException.h"
#include "NotImplementedException.h"
#include "MapFileParser.h"

namespace mtca4u {

  static const std::string MULTIPLEXED_SEQUENCE_PREFIX="AREA_MULTIPLEXED_SEQUENCE_";
  static const std::string SEQUENCE_PREFIX="SEQUENCE_";

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends for multiplexd 2D registers
   */
  template <class UserType>
  class NumericAddressedBackendMuxedRegisterAccessor : public NDRegisterAccessor<UserType> {

    public:

      NumericAddressedBackendMuxedRegisterAccessor(const RegisterPath &registerPathName,
          size_t numberOfElements, size_t elementsOffset, boost::shared_ptr<DeviceBackend> _backend );

      virtual ~NumericAddressedBackendMuxedRegisterAccessor() {}

      void doReadTransfer() override;

      bool doReadTransferNonBlocking() override {
        doReadTransfer();
        return true;
      }

      void postRead() override;

      void write() override;

      void preWrite() override;

      bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const override {
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedBackendMuxedRegisterAccessor<UserType> >(other);
        if(!rhsCasted) return false;
        if(_registerPathName != rhsCasted->_registerPathName) return false;
        if(_ioDevice != rhsCasted->_ioDevice) return false;
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
        throw DeviceException("getFixedPointConverter is not implemented for 2D registers (and deprecated for all "
            "registers).", DeviceException::NOT_IMPLEMENTED);
      }

    protected:

      /** One fixed point converter for each sequence. */
      std::vector< FixedPointConverter > _converters;

      /** The device from (/to) which to perform the DMA transfer */
      boost::shared_ptr<NumericAddressedBackend> _ioDevice;

      /** number of data blocks / samples */
      size_t _nBlocks;

      std::vector<int32_t> _ioBuffer;

      std::vector<RegisterInfoMap::RegisterInfo> _sequenceInfos;

      uint32_t bytesPerBlock;

      /// register and module name
      std::string _moduleName, _registerName;
      RegisterPath _registerPathName;

      /// register address (after restricting to area of interes)
      size_t _bar, _address, _nBytes;

      /// area of interest
      size_t _numberOfElements;
      size_t _elementsOffset;

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {}   // LCOV_EXCL_LINE

  };

  /********************************************************************************************************************/

  template <class UserType>
  NumericAddressedBackendMuxedRegisterAccessor<UserType>::NumericAddressedBackendMuxedRegisterAccessor(
        const RegisterPath &registerPathName, size_t numberOfElements, size_t elementsOffset,
        boost::shared_ptr<DeviceBackend> _backend )
  : NDRegisterAccessor<UserType>(registerPathName),
    _ioDevice(boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend)),
    _registerPathName(registerPathName),
    _numberOfElements(numberOfElements),
    _elementsOffset(elementsOffset)
  {
      // re-split register and module after merging names by the last dot (to allow module.register in the register name)
      _registerPathName.setAltSeparator(".");
      auto moduleAndRegister = MapFileParser::splitStringAtLastDot(_registerPathName.getWithAltSeparator());
      _moduleName = moduleAndRegister.first;
      _registerName = moduleAndRegister.second;

      // build name of area as written in the map file
      std::string areaName = MULTIPLEXED_SEQUENCE_PREFIX+_registerName;

      // Obtain information about the area
      auto registerMapping = _ioDevice->getRegisterMap();
      RegisterInfoMap::RegisterInfo areaInfo;
      registerMapping->getRegisterInfo(areaName, areaInfo, _moduleName);

      // Obtain information for each sequence (= channel) in the area:
      // Create a fixed point converter for each sequence and store the sequence information in a vector
      size_t iSeq = 0;
      while(true) {

        // build name of the next sequence as written in the map file
        RegisterInfoMap::RegisterInfo sequenceInfo;
        std::stringstream sequenceNameStream;
        sequenceNameStream << SEQUENCE_PREFIX << _registerName << "_" << iSeq++;

        // try to find sequence
        try {
          registerMapping->getRegisterInfo(sequenceNameStream.str(), sequenceInfo, _moduleName);
        }
        catch(MapFileException &) {
          // no sequence found: we are done
          break;
        }

        // check consistency
        if(sequenceInfo.nElements != 1) {
          throw MultiplexedDataAccessorException( "Sequence words must have exactly one element",
              MultiplexedDataAccessorException::INVALID_N_ELEMENTS );
        }

        // store sequence info and fixed point converter
        _sequenceInfos.push_back(sequenceInfo);
        _converters.push_back( FixedPointConverter(sequenceInfo.width, sequenceInfo.nFractionalBits, sequenceInfo.signedFlag) );
      }

      // check if no sequences were found
      if(_converters.empty()){
        throw MultiplexedDataAccessorException( "No sequences found for name \""+_registerName+"\".",
            MultiplexedDataAccessorException::EMPTY_AREA );
      }

      // compute size of one block in bytes (one sample for all channels)
      bytesPerBlock = 0;
      for(unsigned int i=0; i<_converters.size(); i++) {
        uint32_t nbt = _sequenceInfos[i].nBytes;
        bytesPerBlock += nbt;
        if(nbt != 1 && nbt != 2 && nbt != 4) {
          throw MultiplexedDataAccessorException( "Sequence word size must correspond to a primitive type",
              MultiplexedDataAccessorException::INVALID_WORD_SIZE );
        }
      }

      // compute number of blocks (number of samples for each channel)
      _nBlocks = std::floor(areaInfo.nBytes / bytesPerBlock);

      // check number of words
      if(_elementsOffset >= _nBlocks) {
        throw DeviceException("Requested element offset exceeds the size of the register!",
            DeviceException::WRONG_PARAMETER);
      }
      if(_numberOfElements == 0) {
        _numberOfElements = _nBlocks - _elementsOffset;
      }
      if(_numberOfElements + _elementsOffset > _nBlocks) {
        throw DeviceException("Requested number of elements exceeds the size of the register!",
            DeviceException::WRONG_PARAMETER);
      }

      // compute the address taking into account the selected area of interest
      _bar = areaInfo.bar;
      _address = areaInfo.address + bytesPerBlock*_elementsOffset;
      _nBytes = bytesPerBlock * _numberOfElements;
      if(_nBytes % sizeof(int32_t) > 0) _nBytes += 4 - _nBytes % sizeof(int32_t); // round up to the next multiple of 4
      if(_nBytes > areaInfo.nBytes) {
        throw DeviceException("Requested number of elements exceeds the size of the register (late, redundant safety check)!",
            DeviceException::WRONG_PARAMETER);
      }
      _nBlocks = _numberOfElements;

      // allocate the buffer for the converted data
      NDRegisterAccessor<UserType>::buffer_2D.resize(_converters.size());
      for(size_t i=0; i<_converters.size(); ++i) {
        NDRegisterAccessor<UserType>::buffer_2D[i].resize(_nBlocks);
      }

      // allocate the raw io buffer
      _ioBuffer.resize(_nBytes/sizeof(int32_t));
  }

  /********************************************************************************************************************/

  template <class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doReadTransfer() {
      _ioDevice->read(_bar, _address, _ioBuffer.data(), _nBytes);
  }

  /********************************************************************************************************************/

  template <class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::postRead() {
      uint8_t *standOfMyioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
      for(size_t blockIndex = 0; blockIndex < _nBlocks; ++blockIndex) {
        for(size_t sequenceIndex = 0; sequenceIndex < _converters.size(); ++sequenceIndex) {
          switch(_sequenceInfos[sequenceIndex].nBytes) {
            case 1: //8 bit variables
              NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex] =
                  _converters[sequenceIndex].template toCooked<UserType>(*(standOfMyioBuffer));
              standOfMyioBuffer++;
              break;
            case 2: //16 bit words
              NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex] =
                  _converters[sequenceIndex].template toCooked<UserType>(*(reinterpret_cast<uint16_t*>(standOfMyioBuffer)));
              standOfMyioBuffer = standOfMyioBuffer + 2;
              break;
            case 4: //32 bit words
              NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex] =
                  _converters[sequenceIndex].template toCooked<UserType>(*(reinterpret_cast<uint32_t*>(standOfMyioBuffer)));
              standOfMyioBuffer = standOfMyioBuffer + 4;
              break;
          }
        }
      }
  }

  /********************************************************************************************************************/

  template <class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::write() {
      preWrite();
      _ioDevice->write(_bar, _address, &(_ioBuffer[0]), _nBytes);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::preWrite() {
      uint8_t *standOfMyioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
      for(size_t blockIndex = 0; blockIndex < _nBlocks; ++blockIndex) {
        for(size_t sequenceIndex = 0; sequenceIndex < _converters.size(); ++sequenceIndex) {
          switch(_sequenceInfos[sequenceIndex].nBytes){
            case 1: //8 bit variables
              *(standOfMyioBuffer) = _converters[sequenceIndex].toRaw(
                  NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex] );
              standOfMyioBuffer++;
              break;
            case 2: //16 bit variables
              *(reinterpret_cast<uint16_t*>(standOfMyioBuffer)) = _converters[sequenceIndex].toRaw(
                  NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex] );
              standOfMyioBuffer = standOfMyioBuffer + 2;
              break;
            case 4: //32 bit variables
              *(reinterpret_cast<uint32_t*>(standOfMyioBuffer)) = _converters[sequenceIndex].toRaw(
                  NDRegisterAccessor<UserType>::buffer_2D[sequenceIndex][blockIndex] );
              standOfMyioBuffer = standOfMyioBuffer + 4;
              break;
          }
        }
      }
  }

}  //namespace mtca4u

#endif // MTCA4U_MEMORY_ADDRESSED_BACKEND_TWO_D_REGISTER_ACCESSOR_H
