#ifndef CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H
#define CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H

#include <fcntl.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "DeviceBackendImpl.h"

namespace ChimeraTK {

/** Base class for address-based device backends (e.g. PICe, Rebot, ...) */
class NumericAddressedBackend : public DeviceBackendImpl {
public:
  NumericAddressedBackend(std::string mapFileName = "");

  virtual ~NumericAddressedBackend() {}

  virtual void read(const std::string &regModule, const std::string &regName,
                    int32_t *data, size_t dataSize = 0,
                    uint32_t addRegOffset = 0);

  virtual void write(const std::string &regModule, const std::string &regName,
                     int32_t const *data, size_t dataSize = 0,
                     uint32_t addRegOffset = 0);

  virtual void read(uint8_t bar, uint32_t address, int32_t *data,
                    size_t sizeInBytes) = 0;

  virtual void write(uint8_t bar, uint32_t address, int32_t const *data,
                     size_t sizeInBytes) = 0;

  virtual std::string readDeviceInfo() = 0;

  /**
   * @brief Determines whether the backend supports merging of requests (read or
   * write)
   *
   * Should return true if the backend supports that several consecutive write
   * or read operations are merged into one single read or write request. If a
   * deriving backend cannot handle such requests, it can prevent this by
   * returning false here.
   *
   * @return true if supported, false otherwise
   */
  virtual bool canMergeRequests() const { return true; }

  boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const;

  std::list<ChimeraTK::RegisterInfoMap::RegisterInfo>
  getRegistersInModule(const std::string &moduleName) const;

  boost::shared_ptr<RegisterInfoMap::RegisterInfo>
  getRegisterInfo(const RegisterPath &registerPathName);

protected:
  /// resolve register name to address with error checks
  void checkRegister(const std::string &regName, const std::string &regModule,
                     size_t dataSize, uint32_t addRegOffset,
                     uint32_t &retDataSize, uint32_t &retRegOff,
                     uint8_t &retRegBar) const;

  /// map from register names to addresses
  boost::shared_ptr<RegisterInfoMap> _registerMap;

  template <typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>>
  getRegisterAccessor_impl(const RegisterPath &registerPathName,
                           size_t numberOfWords, size_t wordOffsetInRegister,
                           AccessModeFlags flags);
};

} // namespace ChimeraTK

#endif /*CHIMERA_TK_MEMORY_ADDRESSED_BACKEND_H*/
