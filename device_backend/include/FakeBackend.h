#ifndef MTCA4U_LIBDEV_FAKE_H
#define MTCA4U_LIBDEV_FAKE_H

#include "DeviceBackendImpl.h"
#include <stdint.h>
#include <stdlib.h>
#include <boost/shared_ptr.hpp>

#define MTCA4U_LIBDEV_BAR_NR 8
#define MTCA4U_LIBDEV_BAR_MEM_SIZE (1024 * 1024)

namespace mtca4u {

class FakeBackend : public DeviceBackendImpl {
private:
  FILE* _pcieMemory;
  std::string _pcieMemoryFileName;
public:
  FakeBackend(std::string host, std::string instance, std::list<std::string> parameters);
  FakeBackend();
  virtual ~FakeBackend();

  virtual void open(const std::string& devName, int perm = O_RDWR,
                       DeviceConfigBase* pConfig = NULL);

  virtual void open();
  virtual void close();

  virtual void readInternal(uint8_t bar, uint32_t address, int32_t* data);
  virtual void writeInternal(uint8_t bar, uint32_t address, int32_t data);

  virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
  virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

  virtual void readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
  virtual void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

  virtual std::string readDeviceInfo();

  static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance, std::list<std::string> parameters);

private:
  /// A private copy constructor, cannot be called from outside.
  /// As the default is not safe and I don't want to implement it right now, I
  /// just make it
  /// private. Make sure not to use it within the class before writing a proper
  /// implemenFakeDevice
  FakeBackend(FakeBackend const&) : DeviceBackendImpl(), _pcieMemory(0), _pcieMemoryFileName() {}

  /// A private assignment operator, cannot be called from outside.
  /// As the default is not safe and I don't want to implement it right now, I
  /// just make it
  /// private. Make sure not to use it within the class before writing a proper
  /// imFakeDeviceation.
  FakeBackend& operator=(FakeBackend const&) { return *this; }
};

} // namespace mtca4u

#endif /* MTCA4U_LIBDEV_STRUCT_H */
