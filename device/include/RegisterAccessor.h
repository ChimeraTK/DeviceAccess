/*
 * RegisterAccessor.h - Non-buffering accessor for device registers
 */

#ifndef MTCA4U_REGISTER_ACCESSOR_H
#define MTCA4U_REGISTER_ACCESSOR_H

#include <typeinfo>

#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"

namespace mtca4u {

  /// forward declaration (only used as tempate arguments)
  class DeviceBackend;

  /** Non-buffering RegisterAccessor class
   *  Allows reading and writing registers with user-provided buffer via plain pointers.
   *  Supports conversion of fixed-point data into standard C data types.
   */
  class RegisterAccessor {

    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getRegisterAccessor() to obtain an instance instead.
       */
      RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer);

      /** Abstract base class needs virtual destructor */
      virtual ~RegisterAccessor();

      /** Read one ore more words from the device. It calls DeviceBackend::readArea,
       * not
       * DeviceBackend::readReg.
       *  @attention In case you leave data size at 0, the full size of the
       * register is read, not just one
       *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
       */
      virtual void readRaw(int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const = 0;

      /** Write one ore more words to the device. It calls DeviceBackend::readArea,
       * not
       * DeviceBackend::writeReg.
       *  @attention In case you leave data size at 0, the full size of the
       * register is read, not just one
       *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
       */
      virtual void writeRaw(int32_t const *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) = 0;

      /** \deprecated
       *  This function is deprecated. Use readRaw() instead!
       *  @todo Add printed runtime warning after release of version 0.2
       */
      virtual void readDMA(int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const = 0;

      /** \deprecated
       *  This function is deprecated. Use writeRaw() instead!
       *  @todo Add printed runtime warning after release of version 0.2
       */
      virtual void writeDMA(int32_t const *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) = 0;

      /** Read (a block of) values with automatic data conversion. The first
       *parameter is a pointer to
       *  to the output buffer. It is templated to work with basic data types.
       *Implementations exist for
       *  \li int32_t
       *  \li uint32_t
       *  \li int16_t
       *  \li uint16_t
       *  \li int8_t
       *  \li uint8_t
       *  \li float
       *  \li double
       *
       *  Note that the input is always a 32 bit word, which is being
       *interpreted
       *to be one
       *  output word. It is not possible to do conversion e.g. from one 32 bit
       *word to two 16 bit values.
       *
       *  @attention Be aware of rounding errors and range overflows, depending
       *on
       *the data type.
       *  \li Rounding to integers is done correctly, so a fixed point value of
       *3.75 would be converted to 4.
       *  \li Coversion to double is guaranteed to be exact (32 bit fixed point
       *with fractional bits
       *  -1024 to 1023 is guaranteed by the FixedPointConverter).
       *  \li Conversion to float is exact for fixed point values up to 24 bits
       *and fractional bits from
       *  -128 to 127.
       */
      template <typename ConvertedDataType>
      void read(ConvertedDataType *convertedData, size_t nWords = 1,
          uint32_t wordOffsetInRegister = 0) const {
        if(nWords == 0) return;
        readImpl(typeid(ConvertedDataType), (void*)convertedData, nWords, wordOffsetInRegister);
      }

      /** Convenience function to read a single word. It allows shorter syntax
       *  as the read value is the return value and one does not have to pass a
       *pointer.
       *
       *  Example: You can use
       *  \code
       *  uint16_t i = registerAccessor.read<uint16_t>();
       *  \endcode
       *  instead of
       *  \code
       *  uint16_t i;
       *  registerAccessor.read(&i);
       *  \endcode
       *  Note that you have to specify the data type as template parameter
       *because return type
       *  overloading is not supported in C++.
       */
      template <typename ConvertedDataType>
      ConvertedDataType read() const {
        ConvertedDataType temp;
        read(&temp);
        return temp;
      }

      /** Write (a block of) words with automatic data conversion. It works for
       *every data
       *  type which has an implicit conversion to double (tested with all data
       *types which are
       *  implemented for read()).
       *  Each input word will be converted to a fixed point integer and written
       *to
       *  a 32 bit register.
       *
       *  @attention Be aware that the conversion to fixed point might come with
       *a
       *loss of
       *  precision or range overflows!
       *
       *  The nWords option does not have a default value to keep the template
       *signature different from
       *  the single word convenience write function.
       */
      template <typename ConvertedDataType>
      void write(ConvertedDataType const *convertedData, size_t nWords,
          uint32_t wordOffsetInRegister = 0) {
        if(nWords == 0) return;
        writeImpl(typeid(ConvertedDataType), (const void*)convertedData, nWords, wordOffsetInRegister);
      }

      /** Convenience function for single words. The value can be given
       * directly,
       * no need to
       *  have a an instance and a pointer for it. This allows code like
       *  \code
       *  registerAccessor.write(0x3F);
       *  \endcode
       *  instead of
       *  \code
       *  static const uint32_t tempValue = 0x3F;
       *  registerAccessor.write(&tempValue); // defaulting nWords to 1 would be
       * possible if this function did not exist
       *  \endcode
       */
      template <typename ConvertedDataType>
      void write(ConvertedDataType const &convertedData) {
        write(&convertedData, 1);
      }

      /** Return number of elements
       */
      virtual unsigned int getNumberOfElements() const = 0;

      /** Returns the register information aka RegisterInfo.
       *  This function was named getRegisterInfo because RegisterInfo will be
       * renamed.
       */
      virtual RegisterInfoMap::RegisterInfo const &getRegisterInfo() const = 0;

      /** Return's a reference to the correctly configured internal fixed point
       *  converter for the register
       */
      virtual FixedPointConverter const &getFixedPointConverter() const = 0;

    protected:

      /** Implementation function for reading converted data
       *  This function is required to achieve runtime polymorphism (virtual function) together with
       *  compiletime polymorphism (templated function). The void* pointer has to be casted into the
       *  proper type specified by the frist argument.
       */
      virtual void readImpl(const std::type_info &type, void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const = 0;

      /** Implementation function for reading converted data
       *  This function is required to achieve runtime polymorphism (virtual function) together with
       *  compiletime polymorphism (templated function). The void* pointer has to be casted into the
       *  proper type specified by the frist argument.
       */
      virtual void writeImpl(const std::type_info &type, const void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) = 0;

      /** Pointer to the device backend used for reading and writing the data
       */
      boost::shared_ptr<DeviceBackend> _deviceBackendPointer;

  };

} // namespace mtca4u

#endif /* MTCA4U_REGISTER_ACCESSOR_H */
