#ifndef _MTCA4U_ADDRESSBASEDDEVICE_BACKEND_H__
#define _MTCA4U_ADDRESSBASEDDEVICE_BACKEND_H__

#include "DeviceBackendImpl.h"
#include "AddressBasedRegisterAccessor.h"
#include "AddressBasedMuxedDataAccessor.h"
#include "MapFileParser.h"
#include "Exception.h"
#include <string>

#include <stdint.h>
#include <fcntl.h>
#include <vector>

namespace mtca4u {

  /** Base class for address-based device backends (e.g. PICe, Rebot, ...)
   */
  class AddressBasedBackend : public DeviceBackendImpl {

    public:

      AddressBasedBackend(std::string mapFileName="");

      virtual ~AddressBasedBackend(){}

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

      virtual void* getRegisterAccessor2Dimpl(const std::type_info &UserType, const std::string &dataRegionName,
          const std::string &module);

  };

} // namespace mtca4u

#endif /*_MTCA4U_ADDRESSBASEDDEVICE_BACKEND_H__*/
