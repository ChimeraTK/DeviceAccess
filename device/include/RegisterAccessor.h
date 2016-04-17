/*
 * RegisterAccessor.h - Non-buffering accessor for device registers
 */

#ifndef MTCA4U_REGISTER_ACCESSOR_H
#define MTCA4U_REGISTER_ACCESSOR_H

#include "VirtualFunctionTemplate.h"
#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"
#include "NDRegisterAccessor.h"
#include "DeviceBackend.h"
#include "NumericAddressedBackend.h"

#include <typeinfo>
#include <boost/fusion/include/at_key.hpp>

namespace mtca4u {

  /// forward declaration (only used as tempate arguments)
  class DeviceBackend;

  /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
   *  \deprecated
   *  This class is deprecated. Use BufferingRegisterAccessor instead!
   *  @todo Add printed runtime warning after release of version 0.8
   */
  class RegisterAccessor {

    public:

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer, const RegisterPath &registerPathName);

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      ~RegisterAccessor();

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      template <typename ConvertedDataType>
      void read(ConvertedDataType *convertedData, size_t nWords = 1, uint32_t wordOffsetInRegister = 0) const {
        if(nWords == 0) return;
        // obtain accessor
	auto & acc =
	  boost::fusion::at_key<ConvertedDataType>(_convertingAccessors.table).instance;
	if (! acc){
	  // The accessor is for reuse. Get the full register size.
	  acc = _dev->getRegisterAccessor<ConvertedDataType>(_registerPathName,
	    _registerInfo->getNumberOfElements(), 0, false); // 0 offset, not raw
	}
	// we have to check the size to protect the following memcpy
	if (nWords+wordOffsetInRegister > acc->accessChannel(0).size() ){
	  throw DeviceException("RegisterAccessor::read Error: reading exceeds the end of register",DeviceException::WRONG_PARAMETER);
	}
        // perform read
        acc->read();
        // copy data to target buffer
        memcpy(convertedData, acc->accessChannel(0).data() + wordOffsetInRegister, nWords*sizeof(ConvertedDataType));
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      template <typename ConvertedDataType>
      ConvertedDataType read() const {
        ConvertedDataType temp;
        read(&temp);
        return temp;
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      template <typename ConvertedDataType>
      void write(ConvertedDataType const *convertedData, size_t nWords, uint32_t wordOffsetInRegister = 0) {
        if(nWords == 0) return;
        // obtain accessor
	auto & acc = 
	  boost::fusion::at_key<ConvertedDataType>(_convertingAccessors.table).instance;
	if (! acc){
	  // The accessor is for reuse. Get the full register size.
	  acc = _dev->getRegisterAccessor<ConvertedDataType>(_registerPathName,
	    _registerInfo->getNumberOfElements(), 0, false); // 0 offset, not raw
	}
	// we have to check the size to protect the following memcpy
	if (nWords+wordOffsetInRegister > acc->accessChannel(0).size() ){
	  throw DeviceException("RegisterAccessor::write Error: writing exceeds the end of register",DeviceException::WRONG_PARAMETER);
	}
        // copy data from source buffer
        memcpy(acc->accessChannel(0).data() + wordOffsetInRegister, convertedData, nWords*sizeof(ConvertedDataType));
        // perform write
        acc->write();
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      template <typename ConvertedDataType>
      void write(ConvertedDataType const &convertedData) {
        write(&convertedData, 1);
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      unsigned int getNumberOfElements() const {
	return _registerInfo->getNumberOfElements();
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      RegisterInfoMap::RegisterInfo getRegisterInfo() const {
        RegisterInfoMap::RegisterInfo info;
        auto castedBackend = boost::dynamic_pointer_cast<NumericAddressedBackend>(_dev);
        if(!castedBackend) {
          throw DeviceException("RegisterAccessor::getRegisterInfo() called for a non-NumericAddressedBackend.",
              DeviceException::NOT_IMPLEMENTED);
        }
        castedBackend->getRegisterMap()->getRegisterInfo(_registerPathName, info);
        return info;
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      FixedPointConverter getFixedPointConverter() const {
 	// it is the converter to int32_t which is meant
        auto & accessor = 
	  boost::fusion::at_key<int32_t>(_convertingAccessors.table).instance;
	if (! accessor){
	  accessor = _dev->getRegisterAccessor<int>(_registerPathName,
						    _registerInfo->getNumberOfElements(),
						    0, false); // 0 offset, not raw
	}
        return accessor->getFixedPointConverter();
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      void readRaw(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const {
        // obtain bytes to copy
        if(dataSize == 0) dataSize = getNumberOfElements() * sizeof(int32_t);
        // check word alignment
        if(dataSize % 4 != 0 || addRegOffset % 4 != 0) {
          throw DeviceException("RegisterAccessor::writeRaw with incorrect word alignment (size and offset must be "
              "dividable by 4)",DeviceException::WRONG_PARAMETER);
        }
	size_t nWords = dataSize/sizeof(int32_t);
	size_t wordOffsetInRegister = addRegOffset/sizeof(int32_t);
	// check accessor and initialise it the first time it is used
	if (! _rawAccessor){
	  // The accessor is for reuse. Get the full register size.
	  _rawAccessor = _dev->getRegisterAccessor<int32_t>(_registerPathName,
	    _registerInfo->getNumberOfElements(), 0, true); // 0 offset, raw
	}
	// we have to check the size to protect the following memcpy
	if (nWords+wordOffsetInRegister > _rawAccessor->accessChannel(0).size() ){
	  throw DeviceException("RegisterAccessor::readRaw Error: reading exceeds the end of register",DeviceException::WRONG_PARAMETER);
	}
        // perform read
        _rawAccessor->read();
        // copy to target buffer
        memcpy(data, _rawAccessor->accessChannel(0).data()+wordOffsetInRegister, dataSize);
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      void writeRaw(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0) {
        // obtain bytes to copy
        if(dataSize == 0) dataSize = getNumberOfElements() * sizeof(int32_t);
        // check word alignment
        if(dataSize % 4 != 0 || addRegOffset % 4 != 0) {
          throw DeviceException("RegisterAccessor::writeRaw with incorrect word alignment (size and offset must be "
              "dividable by 4)",DeviceException::WRONG_PARAMETER);
        }
	size_t nWords = dataSize/sizeof(int32_t);
	size_t wordOffsetInRegister = addRegOffset/sizeof(int32_t);
	// check accessor and initialise it the first time it is used
	if (! _rawAccessor){
	  // The accessor is for reuse. Get the full register size.
	  _rawAccessor = _dev->getRegisterAccessor<int32_t>(_registerPathName,
	    _registerInfo->getNumberOfElements(), 0, true); // 0 offset, raw
	}
	// we have to check the size to protect the following memcpy
	if (nWords+wordOffsetInRegister > _rawAccessor->accessChannel(0).size() ){
	  throw DeviceException("RegisterAccessor::writeRaw Error: writing exceeds the end of register",DeviceException::WRONG_PARAMETER);
	}
        // copy data from source buffer
        memcpy(_rawAccessor->accessChannel(0).data() + wordOffsetInRegister, data, dataSize);
        // perform write
        _rawAccessor->write();
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      void readDMA(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      void writeDMA(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

    protected:

      /** Path name of the register to access */
      RegisterPath _registerPathName;

      /** Pointer to the device backend used for reading and writing the data */
      boost::shared_ptr<DeviceBackend> _dev;

      /** The RegisterInfo for this register */
      boost::shared_ptr< RegisterInfo > _registerInfo;

      /** a "typedef" to be able to use shared_ptr<NDRegisterAccessor> with the TemplateUserTypeMap */
      //template<class UserType>
      //using NDAccessorPtr = boost::shared_ptr<NDRegisterAccessor<UserType> >;
      //
      //sorry, the above code does not work with gcc4.6 (Ubuntu12.4)
      template<class UserType>                                                                                                                                                                               
      class NDAccessorPtr{
      public:
	boost::shared_ptr<NDRegisterAccessor<UserType> > instance;
      };

      /** The converting accessors used under the hood. 
	  They are not initialised in the constructor but only when first used
	  to safe memory. Usually you will not use read or write of all user
	  data types. Thus the variable is mutable to allow initialisation 
	  in the const read and write function.
      */
      mutable TemplateUserTypeMap<NDAccessorPtr> _convertingAccessors;
            
      /** There only is one possible raw accessor: int32_t */
      mutable boost::shared_ptr<NDRegisterAccessor<int32_t> > _rawAccessor;
  };

} // namespace mtca4u

#endif /* MTCA4U_REGISTER_ACCESSOR_H */
