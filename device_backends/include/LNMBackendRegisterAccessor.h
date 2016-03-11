/*
 * LNMBackendRegisterAccessor.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LNM_BACKEND_REGISTER_ACCESSOR_H
#define MTCA4U_LNM_BACKEND_REGISTER_ACCESSOR_H

#include "RegisterAccessor.h"
#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"

namespace mtca4u {

  /// forward declarations
  class DeviceBackend;

  /** Register Accessor implementation for range-type registers of logical name mapping backends
   */
  class LNMBackendRegisterAccessor : public RegisterAccessor {
    public:

      LNMBackendRegisterAccessor(boost::shared_ptr<RegisterAccessor> targetAccessor,
          unsigned int firstIndex, unsigned int length);

      void readRaw(int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      void writeRaw(int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

      RegisterInfoMap::RegisterInfo const &getRegisterInfo() const;

      FixedPointConverter const &getFixedPointConverter() const;

      virtual unsigned int getNumberOfElements() const;

    private:

      /** Underlying register accessor of the target device */
      boost::shared_ptr<RegisterAccessor> _accessor;

      /** First index in the underlying register where our region begins */
      unsigned int _firstIndex;

      /** Length of our register */
      unsigned int _length;

      template <typename ConvertedDataType>
      void read_impl(ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const;
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( LNMBackendRegisterAccessor, read_impl, 3);

      template <typename ConvertedDataType>
      void write_impl(const ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( LNMBackendRegisterAccessor, write_impl, 3);

  };

} // namespace mtca4u

#endif /* MTCA4U_LNM_BACKEND_REGISTER_ACCESSOR_H */
