/*
 * LNMBackendRegisterAccessor.cc
 *
 *  Created on: Feb 10, 2016
 *      Author: Martin Hierholzer
 */

#include "LNMBackendRegisterAccessor.h"
#include "DeviceException.h"

namespace mtca4u {

  /********************************************************************************************************************/
  LNMBackendRegisterAccessor::LNMBackendRegisterAccessor(
        boost::shared_ptr<RegisterAccessor> targetAccessor, unsigned int firstIndex, unsigned int length)
  : RegisterAccessor(boost::shared_ptr<mtca4u::DeviceBackend>()),
    _accessor(targetAccessor),
    _firstIndex(firstIndex),
    _length(length)
  {
  }

  /********************************************************************************************************************/

  void LNMBackendRegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    _accessor->readRaw(data, dataSize, addRegOffset + sizeof(int32_t)*_firstIndex);
  }

  /********************************************************************************************************************/

  void LNMBackendRegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    _accessor->writeRaw(data, dataSize, addRegOffset + sizeof(int32_t)*_firstIndex);
  }

  /********************************************************************************************************************/

  RegisterInfoMap::RegisterInfo const &LNMBackendRegisterAccessor::getRegisterInfo() const {
    throw DeviceException("getRegisterInfo() is not possible with logical name mapping backend accessors.",
        DeviceException::NOT_IMPLEMENTED);
  }

  /********************************************************************************************************************/

  FixedPointConverter const &LNMBackendRegisterAccessor::getFixedPointConverter() const {
    return _accessor->getFixedPointConverter();
  }

  /********************************************************************************************************************/

  unsigned int LNMBackendRegisterAccessor::getNumberOfElements() const {
    return _length;
  }

  /********************************************************************************************************************/

  namespace LogicalNameMappingBackendRangeRegisterAccessorHelper {

    /// Helper class to implement the templated read function with boost::fusion::for_each.
    /// This allows to avoid writing an if-statement for each supported user type.
    class readImplClass
    {
      public:
        readImplClass(const std::type_info &_type, void *_convertedData, size_t _nWords, uint32_t _wordOffsetInRegister,
            boost::shared_ptr<RegisterAccessor> _accessor)
        : type(_type), convertedData(_convertedData), nWords(_nWords), wordOffsetInRegister(_wordOffsetInRegister),
          accessor(_accessor), conversionDone(false)
        {
        }

        /// This function will be called by for_each() for each type in FixedPointConverter::userTypeMap
        template <typename Pair>
        void operator()(Pair) const
        {
          // obtain UserType from given fusion::pair type
          typedef typename Pair::first_type ConvertedDataType;

          // check if UserType is the requested type
          if(typeid(ConvertedDataType) == type) {
            ConvertedDataType *data = reinterpret_cast<ConvertedDataType*>(convertedData);
            accessor->read(data,nWords,wordOffsetInRegister);
            conversionDone = true;
          }
        }

        /// Arguments passed from readImpl
        const std::type_info &type;
        void *convertedData;
        size_t nWords;
        uint32_t wordOffsetInRegister;
        boost::shared_ptr<RegisterAccessor> accessor;

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
            boost::shared_ptr<RegisterAccessor> _accessor)
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
            accessor->write(data,nWords,wordOffsetInRegister);
            conversionDone = true;
          }
        }

        /// Arguments passed from readImpl
        const std::type_info &type;
        const void *convertedData;
        size_t nWords;
        uint32_t wordOffsetInRegister;
        boost::shared_ptr<RegisterAccessor> accessor;

        /// Flag set after the conversion has been performed. This is used to catch invalid types passed to readImpl.
        /// It needs to be mutable since the operator() has to be defined const.
        mutable bool conversionDone;
    };

  }

  /********************************************************************************************************************/

  void LNMBackendRegisterAccessor::readImpl(const std::type_info &type, void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const {
    FixedPointConverter::userTypeMap userTypes;
    LogicalNameMappingBackendRangeRegisterAccessorHelper::readImplClass impl(type,convertedData,nWords,wordOffsetInRegister+_firstIndex,_accessor);
    boost::fusion::for_each(userTypes, impl);
    if(!impl.conversionDone) {
      throw DeviceException("Unknown user type passed to RegisterAccessor::read()", DeviceException::EX_WRONG_PARAMETER);
    }
  }

  /********************************************************************************************************************/

  void LNMBackendRegisterAccessor::writeImpl(const std::type_info &type, const void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) {
    FixedPointConverter::userTypeMap userTypes;
    LogicalNameMappingBackendRangeRegisterAccessorHelper::writeImplClass impl(type,convertedData,nWords,wordOffsetInRegister+_firstIndex,_accessor);
    boost::fusion::for_each(userTypes, impl);
    if(!impl.conversionDone) {
      throw DeviceException("Unknown user type passed to RegisterAccessor::write()", DeviceException::EX_WRONG_PARAMETER);
    }
  }

} // namespace mtca4u
