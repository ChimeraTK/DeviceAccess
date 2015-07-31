#ifndef MTCA4U_LIBDEV_FAKE_H
#define MTCA4U_LIBDEV_FAKE_H

#include "BaseDevice.h"
#include "BaseDeviceImpl.h"
#include <stdint.h>
#include <stdlib.h>

#define MTCA4U_LIBDEV_BAR_NR 8
#define MTCA4U_LIBDEV_BAR_MEM_SIZE (1024 * 1024)

namespace mtca4u {

class FakeDevice : public BaseDeviceImpl {
private:
  std::string _deviceName;
  bool _mapped;
  FILE* pcieMemory;
  std::string pcieMemoryFileName;
  FakeDevice(std::string devName);
public:
  FakeDevice();
  virtual ~FakeDevice();

  virtual void openDev(const std::string& devName, int perm = O_RDWR,
                       DeviceConfigBase* pConfig = NULL);

  virtual void openDev();
  virtual void closeDev();

  virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar);
  virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);

  virtual void readArea(uint32_t regOffset, int32_t* data, size_t size,
                        uint8_t bar);
  virtual void writeArea(uint32_t regOffset, int32_t const* data, size_t size,
                         uint8_t bar);

  virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size,
                       uint8_t bar);
  virtual void writeDMA(uint32_t regOffset, int32_t const* data, size_t size,
                        uint8_t bar);

  virtual void readDeviceInfo(std::string* devInfo);

  //static BaseDevice* createInstance(std::string devName, std::vector<std::string> mappedInfo);
  static BaseDevice* createInstance(std::string devName);

  virtual std::vector<std::string> getDeviceInfo();

private:
  /// A private copy constructor, cannot be called from outside.
  /// As the default is not safe and I don't want to implement it right now, I
  /// just make it
  /// private. Make sure not to use it within the class before writing a proper
  /// implemenFakeDevice
  FakeDevice(FakeDevice const&) : pcieMemory(0), pcieMemoryFileName() {}

  /// A private assignment operator, cannot be called from outside.
  /// As the default is not safe and I don't want to implement it right now, I
  /// just make it
  /// private. Make sure not to use it within the class before writing a proper
  /// imFakeDeviceation.
  FakeDevice& operator=(FakeDevice const&) { return *this; }
};

} // namespace mtca4u

#endif /* MTCA4U_LIBDEV_STRUCT_H */
