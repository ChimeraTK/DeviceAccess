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
#include "ForwardDeclarations.h"

#include <typeinfo>
#include <boost/fusion/include/at_key.hpp>

namespace ChimeraTK {

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
        // obtain the accessor through the handler
        auto & accessorHandler = boost::fusion::at_key<ConvertedDataType>(_convertingAccessorHandlers.table);
        accessorHandler.checkAndResize(nWords, wordOffsetInRegister, false /*not raw*/,
                                       _backend, _registerPathName);
        // now we can be sure that the accessor is valid and has enough memory to perform the memcpy
        accessorHandler.accessor->read();
        // copy data to target buffer. The accessor might not start at index 0, so we have to calculate the correct
        // offset inside the accessor
        memcpy(convertedData, accessorHandler.accessor->accessChannel(0).data() + wordOffsetInRegister-accessorHandler.beginIndex,
          nWords*sizeof(ConvertedDataType));
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
        // obtain the accessor through the handler
        auto & accessorHandler = boost::fusion::at_key<ConvertedDataType>(_convertingAccessorHandlers.table);
        accessorHandler.checkAndResize(nWords, wordOffsetInRegister, false /*not raw*/,
                                       _backend, _registerPathName);
        // now we can be sure that the accessor is valid and has enough memory to perform the memcpy
        // copy data from source buffer to the correct place in the accessor. The accessor does not necessarily
        // start at the beginnig of the register, so we have to correct the offset for this.
        memcpy(accessorHandler.accessor->accessChannel(0).data() + wordOffsetInRegister-accessorHandler.beginIndex,
               convertedData, nWords*sizeof(ConvertedDataType));
        // perform write
        accessorHandler.accessor->write();
      }

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8       */
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
        auto castedBackend = boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend);
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
        // We use the double accessor, which is the most likely to exist
        auto & accessorHandler = boost::fusion::at_key<double>(_convertingAccessorHandlers.table);
        //In case we have to allocate, use the smallest possible accessor to be memory and transfer efficient.
        //(about the offset we can just guess that 0 is fine.)
        accessorHandler.checkAndResize(1, 0, false /*not raw*/, _backend, _registerPathName);

        return accessorHandler.accessor->getFixedPointConverter();
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

        _rawAccessorHandler.checkAndResize(nWords, wordOffsetInRegister, true /*raw*/,
                                           _backend, _registerPathName);
        // perform read
        _rawAccessorHandler.accessor->read();
        // copy to target buffer
        memcpy(data, _rawAccessorHandler.accessor->accessChannel(0).data()+wordOffsetInRegister-_rawAccessorHandler.beginIndex, dataSize);
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

        _rawAccessorHandler.checkAndResize(nWords, wordOffsetInRegister, true /*raw*/,
                                           _backend, _registerPathName);
        // copy data from source buffer
        memcpy(_rawAccessorHandler.accessor->accessChannel(0).data() + wordOffsetInRegister-_rawAccessorHandler.beginIndex, data, dataSize);
        // perform write
        _rawAccessorHandler.accessor->write();
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
      boost::shared_ptr<DeviceBackend> _backend;

      /** The RegisterInfo for this register */
      boost::shared_ptr< RegisterInfo > _registerInfo;

      template<class UserType>
      class AccessorHandler{
      public:
        AccessorHandler() : beginIndex(0),endIndex(0) {}

        boost::shared_ptr<NDRegisterAccessor<UserType> > accessor;
        size_t beginIndex; // the first index of the accessor;
        size_t endIndex; // the end index (first index after the valid range, liken end iterator)
        void checkAndResize(size_t nWords, size_t wordOffsetInRegister, bool isRaw, boost::shared_ptr<DeviceBackend> const & backend, RegisterPath const & registerPathName){
          // no need for out of register size checking. getRegisterAccessor does the job for us and throws in case of error.
          if ( (wordOffsetInRegister < beginIndex) || (wordOffsetInRegister+nWords > (endIndex) || !accessor)){
              // we have to (re)allocate
              size_t newBeginIndex;
              if (!accessor){
                  newBeginIndex = wordOffsetInRegister;
              }else{
                  newBeginIndex = std::min(wordOffsetInRegister, beginIndex);
              }
            size_t newEndIndex = std::max(wordOffsetInRegister+nWords, endIndex);
            accessor = backend->getRegisterAccessor<UserType>(registerPathName, newEndIndex-newBeginIndex, newBeginIndex, isRaw); // 0 offset, raw
            // we have to update the accessor with the current hardware information. It might be that a partial write is going to happen.
            accessor->read();
            // only if creating the new accessor succeeds we change the index bookkeeping variables
            beginIndex = newBeginIndex;
            endIndex = newEndIndex;
          }
        }
      };

      /** The converting accessors used under the hood. 
          They are not initialised in the constructor but only when first used
          to safe memory. Usually you will not use read or write of all user
          data types. Thus the variable is mutable to allow initialisation 
          in the const read and write function.
      */
      mutable TemplateUserTypeMap<AccessorHandler> _convertingAccessorHandlers;
            
      /** There only is one possible raw accessor: int32_t */
      mutable AccessorHandler<int32_t> _rawAccessorHandler;
  };

} // namespace ChimeraTK

#endif /* MTCA4U_REGISTER_ACCESSOR_H */
