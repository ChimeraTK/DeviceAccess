/*
 * LogicalNameMappingBackendRangeRegisterAccessor.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_RANGE_REGISTER_ACCESSOR_H
#define MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_RANGE_REGISTER_ACCESSOR_H

#include "RegisterAccessor.h"
#include "FixedPointConverter.h"
#include "RegisterInfoMap.h"

namespace mtca4u {

  /// forward declarations
  class DeviceBackend;

  /** Register Accessor implementation for range-type registers of logical name mapping backends
   */
  class LogicalNameMappingBackendRangeRegisterAccessor : public RegisterAccessor {
    public:

      LogicalNameMappingBackendRangeRegisterAccessor(boost::shared_ptr<RegisterAccessor> targetAccessor,
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

      virtual void readImpl(const std::type_info &type, void *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const;

      virtual void writeImpl(const std::type_info &type, const void *convertedData, size_t nWords, uint32_t wordOffsetInRegister);

  };

} // namespace mtca4u

#endif /* MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_RANGE_REGISTER_ACCESSOR_H */
