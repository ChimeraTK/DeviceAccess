#ifndef _MTCA4U_BASE_DEVICE_H__
#define _MTCA4U_BASE_DEVICE_H__

#include "DeviceConfigBase.h"
#include "Exception.h"
#include <string>
#include <stdint.h>
#include <fcntl.h>
#include <vector>

namespace mtca4u {

class DeviceException : public Exception {
public:
	DeviceException(const std::string &message, unsigned int exceptionID)
	: Exception( message, exceptionID ){};
};

/** The base class of an IO device.
 */
class BaseDevice {

public:
  // Todo move this function to private space.
  virtual void open(const std::string& devName, int perm = O_RDWR,
                       DeviceConfigBase* pConfig = NULL) = 0;
  virtual void open() = 0;
  virtual void close() = 0;

  virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar) = 0;
  virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar) = 0;

  virtual void readArea(uint32_t regOffset, int32_t* data, size_t size,
                        uint8_t bar) = 0;
  virtual void writeArea(uint32_t regOffset, int32_t const* data, size_t size,
                         uint8_t bar) = 0;

  virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size,
                       uint8_t bar) = 0;
  virtual void writeDMA(uint32_t regOffset, int32_t const* data, size_t size,
                        uint8_t bar) = 0;

  virtual std::string readDeviceInfo() = 0;
  /** Return whether a device has been opened or not.
   *  As the variable already exists in the base class we implement this
   * function here to avoid
   *  having to reimplement the same, trivial return function over and over
   * again.
   */
  virtual bool isOpen() = 0;
  /** Return whether a device has been connected or not.
   * A device is considered connected when it is created.
   */
  virtual bool isConnected() = 0;
  /** Return whether a device has been disconnected.
   * A device is considered connected when it is distroyed.
        */
};

} // namespace mtca4u

#endif /*_MTCA4U_BASE_DEVICE_H__*/
