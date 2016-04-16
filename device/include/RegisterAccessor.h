/*
 * RegisterAccessor.h - Non-buffering accessor for device registers
 */

#ifndef MTCA4U_REGISTER_ACCESSOR_H
#define MTCA4U_REGISTER_ACCESSOR_H

#include <typeinfo>

#include "VirtualFunctionTemplate.h"
#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"
#include "NDRegisterAccessor.h"
#include "DeviceBackend.h"
#include "NumericAddressedBackend.h"

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
        auto acc = _dev->getRegisterAccessor<ConvertedDataType>(_registerPathName, nWords, wordOffsetInRegister, false);
        // perform read
        acc->read();
        // copy data to target buffer
        memcpy(convertedData, acc->accessChannel(0).data(), nWords*sizeof(ConvertedDataType));
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
        auto acc = _dev->getRegisterAccessor<ConvertedDataType>(_registerPathName, nWords, wordOffsetInRegister, false);
        // copy data from source buffer
        memcpy(acc->accessChannel(0).data(), convertedData, nWords*sizeof(ConvertedDataType));
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
        auto acc = _dev->getRegisterAccessor<int32_t>(_registerPathName, 1, 0, false);
        return acc->getFixedPointConverter();
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
        // obtain accessor
        auto acc = _dev->getRegisterAccessor<int32_t>(_registerPathName, dataSize/sizeof(int32_t),
            addRegOffset/sizeof(int32_t), true);
        // perform read
        acc->read();
        // copy to target buffer
        memcpy(data, acc->accessChannel(0).data(), dataSize);
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
        // obtain accessor
        auto acc = _dev->getRegisterAccessor<int32_t>(_registerPathName, dataSize/sizeof(int32_t),
            addRegOffset/sizeof(int32_t), true);
        // copy data from source buffer
        memcpy(acc->accessChannel(0).data(), data, dataSize);
        // perform write
        acc->write();
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

      /// only temporary while developing. will be a map with all types
      boost::shared_ptr<NDRegisterAccessor<int> > _intAccessor;

  };

  template <>
  void RegisterAccessor::read<int>(int *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const;

} // namespace mtca4u

#endif /* MTCA4U_REGISTER_ACCESSOR_H */
