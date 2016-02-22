/*
 * AddressBasedRegisterAccessor.cc
 *
 *  Created on: Feb 2, 2016
 *      Author: Martin Hierholzer
 */

#define FUSION_MAX_MAP_SIZE 20
#define FUSION_MAX_VECTOR_SIZE 20
#include <boost/fusion/algorithm.hpp>

#include "MemoryAddressedBackendRegisterAccessor.h"
#include "MemoryAddressedBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  /********************************************************************************************************************/
  MemoryAddressedBackendRegisterAccessor::MemoryAddressedBackendRegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
        boost::shared_ptr<DeviceBackend> deviceBackendPointer)
  : RegisterAccessor(deviceBackendPointer),
    _registerInfo(registerInfo),
    _fixedPointConverter(_registerInfo.width, _registerInfo.nFractionalBits, _registerInfo.signedFlag)
  {}

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::checkRegister(const RegisterInfoMap::RegisterInfo &registerInfo,
      size_t dataSize,
      uint32_t addRegOffset,
      uint32_t &retDataSize,
      uint32_t &retRegOff) {
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4",
          DeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4",
            DeviceException::EX_WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.nBytes - addRegOffset) {
        throw DeviceException("Data size exceed register size",
            DeviceException::EX_WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.nBytes;
    }
    retRegOff = registerInfo.address + addRegOffset;
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->read(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    uint32_t retDataSize;
    uint32_t retRegOff;
    checkRegister(_registerInfo, dataSize, addRegOffset, retDataSize, retRegOff);
    _deviceBackendPointer->write(_registerInfo.bar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  RegisterInfoMap::RegisterInfo const &MemoryAddressedBackendRegisterAccessor::getRegisterInfo() const {
    return _registerInfo;
  }

  /********************************************************************************************************************/

  FixedPointConverter const &MemoryAddressedBackendRegisterAccessor::getFixedPointConverter() const {
    return _fixedPointConverter;
  }

  /********************************************************************************************************************/

  unsigned int MemoryAddressedBackendRegisterAccessor::getNumberOfElements() const {
    return _registerInfo.nElements;
  }

  /********************************************************************************************************************/

  namespace MemoryAddressedBackendRegisterAccessorHelper {

    /// Helper class to implement the templated read function with boost::fusion::for_each.
    /// This allows to avoid writing an if-statement for each supported user type.
    class readImplClass
    {
      public:
        readImplClass(const std::type_info &_type, void *_convertedData, size_t _nWords, uint32_t _wordOffsetInRegister,
                      const MemoryAddressedBackendRegisterAccessor *_accessor)
        : type(_type), convertedData(_convertedData), nWords(_nWords), wordOffsetInRegister(_wordOffsetInRegister),
          accessor(_accessor), conversionDone(false)
        {}

        /// This function will be called by for_each() for each type in FixedPointConverter::userTypeMap
        template <typename Pair>
        void operator()(Pair) const
        {
          // obtain UserType from given fusion::pair type
          typedef typename Pair::first_type ConvertedDataType;

          // check if UserType is the requested type
          if(typeid(ConvertedDataType) == type) {
            std::vector<int32_t> rawDataBuffer(nWords);
            accessor->readRaw(&(rawDataBuffer[0]), nWords*sizeof(int32_t), wordOffsetInRegister*sizeof(int32_t));
            ConvertedDataType *data = reinterpret_cast<ConvertedDataType*>(convertedData);
            for(size_t i=0; i < nWords; ++i){
              data[i] = accessor->getFixedPointConverter().template toCooked<ConvertedDataType>(rawDataBuffer[i]);
            }
            conversionDone = true;
          }
        }

        /// Arguments passed from readImpl
        const std::type_info &type;
        void *convertedData;
        size_t nWords;
        uint32_t wordOffsetInRegister;
        const MemoryAddressedBackendRegisterAccessor *accessor;

        /// Flag set after the conversion has been performed. This is used to catch invalid types passed to readImpl.
        /// It needs to be mutable since the operator() has to be defined const.
        mutable bool conversionDone;
    };

    /// Helper class to implement the templated write function with boost::fusion::for_each.
    /// This allows to avoid writing an if-statement for each supported user type.
    class writeImplClass
    {
      public:
        writeImplClass(const std::type_info &_type, const void *_convertedData, size_t _nWords, uint32_t _wordOffsetInRegister,
                      MemoryAddressedBackendRegisterAccessor *_accessor)
        : type(_type), convertedData(_convertedData), nWords(_nWords), wordOffsetInRegister(_wordOffsetInRegister),
          accessor(_accessor), conversionDone(false)
        {}

        /// This function will be called by for_each() for each type in FixedPointConverter::userTypeMap
        template <typename Pair>
        void operator()(Pair) const
        {
          // obtain UserType from given fusion::pair type
          typedef typename Pair::first_type ConvertedDataType;

          // check if UserType is the requested type
          if(typeid(ConvertedDataType) == type) {
            const ConvertedDataType *data = reinterpret_cast<const ConvertedDataType*>(convertedData);
            std::vector<int32_t> rawDataBuffer(nWords);
            for (size_t i = 0; i < nWords; ++i) {
              rawDataBuffer[i] = accessor->getFixedPointConverter().toRaw(data[i]);
            }
            accessor->writeRaw(&(rawDataBuffer[0]), nWords * sizeof(int32_t),
                 wordOffsetInRegister * sizeof(int32_t));
            conversionDone = true;
          }
        }

        /// Arguments passed from readImpl
        const std::type_info &type;
        const void *convertedData;
        size_t nWords;
        uint32_t wordOffsetInRegister;
        MemoryAddressedBackendRegisterAccessor *accessor;

        /// Flag set after the conversion has been performed. This is used to catch invalid types passed to readImpl.
        /// It needs to be mutable since the operator() has to be defined const.
        mutable bool conversionDone;
    };

  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::readImpl(const std::type_info &type, void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const {
    FixedPointConverter::userTypeMap userTypes;
    MemoryAddressedBackendRegisterAccessorHelper::readImplClass impl(type,convertedData,nWords,wordOffsetInRegister,this);
    boost::fusion::for_each(userTypes, impl);
    if(!impl.conversionDone) {
      throw DeviceException("Unknown user type passed to RegisterAccessor::read()", DeviceException::EX_WRONG_PARAMETER);
    }
  }

  /********************************************************************************************************************/

  void MemoryAddressedBackendRegisterAccessor::writeImpl(const std::type_info &type, const void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) {
    FixedPointConverter::userTypeMap userTypes;
    MemoryAddressedBackendRegisterAccessorHelper::writeImplClass impl(type,convertedData,nWords,wordOffsetInRegister,this);
    boost::fusion::for_each(userTypes, impl);
    if(!impl.conversionDone) {
      throw DeviceException("Unknown user type passed to RegisterAccessor::write()", DeviceException::EX_WRONG_PARAMETER);
    }
  }

} // namespace mtca4u
