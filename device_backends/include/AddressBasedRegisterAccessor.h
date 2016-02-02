/*
 * RegisterAccessor.h - Non-buffering accessor for device registers
 */

#ifndef ADDRESSBASEDADDRESSBASEDREGISTERACCESSOR_H
#define ADDRESSBASEDADDRESSBASEDREGISTERACCESSOR_H

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
  class AddressBasedRegisterAccessor : public RegisterAccessor {
    public:

      AddressBasedRegisterAccessor(const RegisterInfoMap::RegisterInfo &registerInfo,
        boost::shared_ptr<DeviceBackend> deviceBackendPointer);

      void readRaw(int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const;

      void writeRaw(int32_t const *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0);

      void readDMA(int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const;

      void writeDMA(int32_t const *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0);

      RegisterInfoMap::RegisterInfo const &getRegisterInfo() const;

      FixedPointConverter const &getFixedPointConverter() const;

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

      virtual void readImpl(const std::type_info &type, void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const;

      virtual void writeImpl(const std::type_info &type, const void *convertedData, size_t nWords, uint32_t wordOffsetInRegister);

  };

} // namespace mtca4u

#endif /* ADDRESSBASEDADDRESSBASEDREGISTERACCESSOR_H */
