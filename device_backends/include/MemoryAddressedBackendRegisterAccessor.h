/*
 * MemoryAddressedBackendRegisterAccessor.h - Non-buffering accessor for device registers
 */

#ifndef MTCA4U_MEMORY_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H
#define MTCA4U_MEMORY_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H

#include "RegisterAccessor.h"
#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"

namespace mtca4u {

  /// forward declarations
  class DeviceBackend;

  /** Non-buffering RegisterAccessor class
   *  Allows reading and writing registers with user-provided buffer via plain pointers.
   *  Supports conversion of fixed-point data into standard C data types.
   */
  class MemoryAddressedBackendRegisterAccessor : public RegisterAccessor {
    public:

      MemoryAddressedBackendRegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
        boost::shared_ptr<DeviceBackend> deviceBackendPointer);

      void readRaw(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      void writeRaw(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

      RegisterInfoMap::RegisterInfo const &getRegisterInfo() const;

      FixedPointConverter const &getFixedPointConverter() const;

      FixedPointConverter &getFixedPointConverter();

      virtual unsigned int getNumberOfElements() const;

    private:

      /** Address, size and fixed-point representation information of the register from the map file
       */
      RegisterInfoMap::RegisterInfo _registerInfo;

      /** Fixed point converter to interpret the data
       */
      FixedPointConverter _fixedPointConverter;

      /** Address conversion and check
       */
      static void checkRegister(const RegisterInfoMap::RegisterInfo &registerInfo, size_t dataSize,
          uint32_t addRegOffset, uint32_t &retDataSize, uint32_t &retRegOff);

      template <typename ConvertedDataType>
      void read_impl(ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const;
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( MemoryAddressedBackendRegisterAccessor, read_impl, 3);

      template <typename ConvertedDataType>
      void write_impl(const ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( MemoryAddressedBackendRegisterAccessor, write_impl, 3);

  };

} // namespace mtca4u

#endif /* MTCA4U_MEMORY_ADDRESSED_BACKEND_REGISTER_ACCESSOR_H */
