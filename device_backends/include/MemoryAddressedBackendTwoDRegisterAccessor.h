#ifndef MTCA4U_MEMORY_ADDRESSED_BACKEND_TWO_D_REGISTER_ACCESSOR_H
#define MTCA4U_MEMORY_ADDRESSED_BACKEND_TWO_D_REGISTER_ACCESSOR_H

#include <sstream>
#include <boost/shared_ptr.hpp>

#include "TwoDRegisterAccessorImpl.h"
#include "RegisterInfoMap.h"
#include "FixedPointConverter.h"
#include "DeviceBackend.h"
#include "Exception.h"
#include "MapException.h"
#include "NotImplementedException.h"

namespace mtca4u {

  template<class UserType, class SequenceWordType>
  class SequenceDeMultiplexerTest;

  template<class UserType>
  class MixedTypeTest;

  typedef RegisterInfoMap::RegisterInfo SequenceInfo;

  static const std::string MULTIPLEXED_SEQUENCE_PREFIX="AREA_MULTIPLEXED_SEQUENCE_";
  static const std::string SEQUENCE_PREFIX="SEQUENCE_";

  template <class UserType>
  class MemoryAddressedBackendTwoDRegisterAccessor : public TwoDRegisterAccessorImpl<UserType> {

    public:

      MemoryAddressedBackendTwoDRegisterAccessor( const std::string &_dataRegionName, const std::string &_module,
          boost::shared_ptr<DeviceBackend> _backend );

      virtual ~MemoryAddressedBackendTwoDRegisterAccessor() {}

      void read();

      void write();

      size_t getNumberOfDataSequences();

      /// TODO this function is not in the interface, can we remove it?
      uint32_t getSizeOneBlock() {
        return bytesPerBlock/4;
      }

      virtual bool operator==(const TransferElement &rightHandSide) const {
        auto rhsCasted = dynamic_cast<const MemoryAddressedBackendTwoDRegisterAccessor<UserType>*>(&rightHandSide);
        if(!rhsCasted) return false;
        if(_registerName != rhsCasted->_registerName) return false;
        if(_moduleName != rhsCasted->_moduleName) return false;
        return true;
      }

    protected:

      /** One fixed point converter for each sequence. */
      std::vector< FixedPointConverter > _converters;

      void fillSequences();

      void fillIO_Buffer();

      std::vector<int32_t> _ioBuffer;

      SequenceInfo _areaInfo;

      std::vector<SequenceInfo> _sequenceInfos;

      uint32_t bytesPerBlock;

      friend class MixedTypeTest<UserType>;

      /// register and module name
      std::string _registerName, _moduleName;

  };

  /********************************************************************************************************************/

  // TODO move this class to the test .cc file
  template<class UserType>
  class MixedTypeTest {

    public:

      MixedTypeTest(MemoryAddressedBackendTwoDRegisterAccessor < UserType > *mixedTypeInstance = NULL)
      : _mixedTypeInstance(mixedTypeInstance)
      {};

      uint32_t getSizeOneBlock() {
        return _mixedTypeInstance->bytesPerBlock / 4;
      }
      size_t getNBlock() {
        return _mixedTypeInstance->_nBlocks;
      }
      size_t getConvertersSize() {
        return (_mixedTypeInstance->_converters).size();
      }
      int32_t getIOBUffer(uint index) {
        return _mixedTypeInstance->_ioBuffer[index];
      }

    private:

      MemoryAddressedBackendTwoDRegisterAccessor<UserType> *_mixedTypeInstance;
  };

  /********************************************************************************************************************/

  template <class UserType>
  MemoryAddressedBackendTwoDRegisterAccessor<UserType>::MemoryAddressedBackendTwoDRegisterAccessor(
      const std::string &_dataRegionName, const std::string &moduleName, boost::shared_ptr<DeviceBackend> _backend )
  : TwoDRegisterAccessorImpl<UserType>(_backend), _registerName(_dataRegionName), _moduleName(moduleName)
  {
      // build name of area as written in the map file
      std::string areaName = MULTIPLEXED_SEQUENCE_PREFIX+_dataRegionName;

      // Obtain information about the area
      auto registerMapping = _backend->getRegisterMap();
      registerMapping->getRegisterInfo(areaName, _areaInfo, moduleName);

      // Obtain information for each sequence (= channel) in the area:
      // Create a fixed point converter for each sequence and store the sequence information in a vector
      size_t iSeq = 0;
      while(true) {

        // build name of the next sequence as written in the map file
        SequenceInfo sequenceInfo;
        std::stringstream sequenceNameStream;
        sequenceNameStream << SEQUENCE_PREFIX << _dataRegionName << "_" << iSeq++;

        // try to find sequence
        try {
          registerMapping->getRegisterInfo(sequenceNameStream.str(), sequenceInfo, moduleName);
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
        throw MultiplexedDataAccessorException( "No sequences found for name \""+_dataRegionName+"\".",
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
      TwoDRegisterAccessorImpl<UserType>::_nBlocks = std::floor(_areaInfo.nBytes / bytesPerBlock);

      // allocate the buffer for the converted data
      TwoDRegisterAccessorImpl<UserType>::_sequences.resize(_converters.size());
      for (size_t i=0; i<_converters.size(); ++i) {
        TwoDRegisterAccessorImpl<UserType>::_sequences[i].resize(TwoDRegisterAccessorImpl<UserType>::_nBlocks);
      }

      // allocate the raw io buffer
      _ioBuffer.resize(_areaInfo.nBytes);
  }

  /********************************************************************************************************************/

  template <class UserType>
  void MemoryAddressedBackendTwoDRegisterAccessor<UserType>::read() {
      TwoDRegisterAccessorImpl<UserType>::_ioDevice->read(_areaInfo.bar, _areaInfo.address, _ioBuffer.data(), _areaInfo.nBytes);
      fillSequences();
  }

  /********************************************************************************************************************/

  template <class UserType>
  void MemoryAddressedBackendTwoDRegisterAccessor<UserType>::fillSequences() {
      uint8_t *standOfMyioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
      for(size_t blockIndex = 0; blockIndex < TwoDRegisterAccessorImpl<UserType>::_nBlocks; ++blockIndex) {
        for(size_t sequenceIndex = 0; sequenceIndex < _converters.size(); ++sequenceIndex) {
          switch(_sequenceInfos[sequenceIndex].nBytes) {
            case 1: //8 bit variables
              TwoDRegisterAccessorImpl<UserType>::_sequences[sequenceIndex][blockIndex] =
                  _converters[sequenceIndex].template toCooked<UserType>(*(standOfMyioBuffer));
              standOfMyioBuffer++;
              break;
            case 2: //16 bit words
              TwoDRegisterAccessorImpl<UserType>::_sequences[sequenceIndex][blockIndex] =
                  _converters[sequenceIndex].template toCooked<UserType>(*(reinterpret_cast<uint16_t*>(standOfMyioBuffer)));
              standOfMyioBuffer = standOfMyioBuffer + 2;
              break;
            case 4: //32 bit words
              TwoDRegisterAccessorImpl<UserType>::_sequences[sequenceIndex][blockIndex] =
                  _converters[sequenceIndex].template toCooked<UserType>(*(reinterpret_cast<uint32_t*>(standOfMyioBuffer)));
              standOfMyioBuffer = standOfMyioBuffer + 4;
              break;
          }
        }
      }
  }

  /********************************************************************************************************************/

  template <class UserType>
  void MemoryAddressedBackendTwoDRegisterAccessor<UserType>::write() {
      fillIO_Buffer();

      TwoDRegisterAccessorImpl<UserType>::_ioDevice->write(
          _areaInfo.bar,
          _areaInfo.address, &(_ioBuffer[0]),
          _areaInfo.nBytes);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void MemoryAddressedBackendTwoDRegisterAccessor<UserType>::fillIO_Buffer() {
      uint8_t *standOfMyioBuffer = reinterpret_cast<uint8_t*>(&_ioBuffer[0]);
      for(size_t blockIndex = 0; blockIndex < TwoDRegisterAccessorImpl<UserType>::_nBlocks; ++blockIndex) {
        for(size_t sequenceIndex = 0; sequenceIndex < _converters.size(); ++sequenceIndex) {
          switch(_sequenceInfos[sequenceIndex].nBytes){
            case 1: //8 bit variables
              *(standOfMyioBuffer) = _converters[sequenceIndex].toRaw(
                  TwoDRegisterAccessorImpl<UserType>::_sequences[sequenceIndex][blockIndex] );
              standOfMyioBuffer++;
              break;
            case 2: //16 bit variables
              *(reinterpret_cast<uint16_t*>(standOfMyioBuffer)) = _converters[sequenceIndex].toRaw(
                  TwoDRegisterAccessorImpl<UserType>::_sequences[sequenceIndex][blockIndex] );
              standOfMyioBuffer = standOfMyioBuffer + 2;
              break;
            case 4: //32 bit variables
              *(reinterpret_cast<uint32_t*>(standOfMyioBuffer)) = _converters[sequenceIndex].toRaw(
                  TwoDRegisterAccessorImpl<UserType>::_sequences[sequenceIndex][blockIndex] );
              standOfMyioBuffer = standOfMyioBuffer + 4;
              break;
          }
        }
      }
  }

  /********************************************************************************************************************/

  template <class UserType>
  size_t MemoryAddressedBackendTwoDRegisterAccessor<UserType>::getNumberOfDataSequences() {
      return (TwoDRegisterAccessorImpl<UserType>::_sequences.size());
  }

}  //namespace mtca4u

#endif // MTCA4U_MEMORY_ADDRESSED_BACKEND_TWO_D_REGISTER_ACCESSOR_H
