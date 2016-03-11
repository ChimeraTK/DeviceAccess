#ifndef MTCA4U_MEMORY_ADDRESSED_BACKEND_H
#define MTCA4U_MEMORY_ADDRESSED_BACKEND_H

#include <string>
#include <stdint.h>
#include <fcntl.h>
#include <vector>

#include "DeviceBackendImpl.h"
#include "MemoryAddressedBackendTwoDRegisterAccessor.h"
#include "MemoryAddressedBackendRegisterAccessor.h"
#include "MapFileParser.h"
#include "Exception.h"

namespace mtca4u {

  /** Base class for address-based device backends (e.g. PICe, Rebot, ...)
   */
  class MemoryAddressedBackend : public DeviceBackendImpl {

    public:

      MemoryAddressedBackend(std::string mapFileName="");

      virtual ~MemoryAddressedBackend(){}

      virtual void read(const std::string &regModule, const std::string &regName,
          int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

      virtual void write(const std::string &regModule, const std::string &regName,
          int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) = 0;

      virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) = 0;

      virtual std::string readDeviceInfo() = 0;

      virtual boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(
          const std::string &registerName,
          const std::string &module = std::string());

      virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const;

      virtual std::list<mtca4u::RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &moduleName) const;

      virtual std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > getRegisterAccessorsInModule(
          const std::string &moduleName);

    protected:

      /// resolve register name to address with error checks
      void checkRegister(const std::string &regName,
          const std::string &regModule, size_t dataSize,
          uint32_t addRegOffset, uint32_t &retDataSize,
          uint32_t &retRegOff, uint8_t &retRegBar) const;

      /// map from register names to addresses
      boost::shared_ptr<RegisterInfoMap> _registerMap;

      template<typename UserType>
      boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > getTwoDRegisterAccessor_impl(const std::string &registerName,
          const std::string &module);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( MemoryAddressedBackend, getTwoDRegisterAccessor_impl, 2);

      virtual void setRegisterMap(boost::shared_ptr<RegisterInfoMap> registerMap) // LCOV_EXCL_LINE only for compatibility!
      { // LCOV_EXCL_LINE only for compatibility!
        _registerMap = registerMap; // LCOV_EXCL_LINE only for compatibility!
        _catalogue = _registerMap->getRegisterCatalogue(); // LCOV_EXCL_LINE only for compatibility!
      } // LCOV_EXCL_LINE only for compatibility!

  };

} // namespace mtca4u

#endif /*MTCA4U_MEMORY_ADDRESSED_BACKEND_H*/
