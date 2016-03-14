/*
 * RegisterAccessor.h - Non-buffering accessor for device registers
 */

#ifndef MTCA4U_REGISTER_ACCESSOR_H
#define MTCA4U_REGISTER_ACCESSOR_H

#include <typeinfo>

#include "VirtualFunctionTemplate.h"
#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"

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
      RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer);

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual ~RegisterAccessor();

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      template <typename ConvertedDataType>
      void read(ConvertedDataType *convertedData, size_t nWords = 1, uint32_t wordOffsetInRegister = 0) const {
        if(nWords == 0) return;
        CALL_VIRTUAL_FUNCTION_TEMPLATE(read_impl, ConvertedDataType, convertedData, nWords, wordOffsetInRegister);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE( read_impl, void(T*, size_t, uint32_t) );

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
        CALL_VIRTUAL_FUNCTION_TEMPLATE(write_impl, ConvertedDataType, convertedData, nWords, wordOffsetInRegister);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE( write_impl, void(const T*, size_t, uint32_t) );

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
      virtual unsigned int getNumberOfElements() const = 0;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual RegisterInfoMap::RegisterInfo const &getRegisterInfo() const = 0;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual FixedPointConverter const &getFixedPointConverter() const = 0;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual FixedPointConverter &getFixedPointConverter() = 0;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void readRaw(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const = 0;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void writeRaw(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void readDMA(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      /** \brief DEPRECATED! Use BufferingRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use BufferingRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void writeDMA(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

    protected:

      /** Pointer to the device backend used for reading and writing the data
       */
      boost::shared_ptr<DeviceBackend> _deviceBackendPointer;

  };

} // namespace mtca4u

#endif /* MTCA4U_REGISTER_ACCESSOR_H */
