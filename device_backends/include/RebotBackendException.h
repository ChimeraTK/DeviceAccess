#ifndef CHIMERATK_REBOT_BACKEND_EXCEPTION_H
#define CHIMERATK_REBOT_BACKEND_EXCEPTION_H

#include <string>

#include "DeviceBackendException.h"

namespace ChimeraTK {
  using namespace mtca4u;
  
  /// Provides class for exceptions related to RebotDevice
  class RebotBackendException : public DeviceBackendException {
    public:
      enum {
        EX_OPEN_SOCKET,
        EX_CONNECTION_FAILED,
        EX_CLOSE_SOCKET_FAILED,
        EX_SOCKET_WRITE_FAILED,
        EX_SOCKET_READ_FAILED,
        EX_DEVICE_CLOSED,
        EX_SET_IP_FAILED,
        EX_SET_PORT_FAILED,
        EX_SIZE_INVALID,
        EX_INVALID_PARAMETERS,
        EX_INVALID_REGISTER_ADDRESS
      };
      RebotBackendException(const std::string &_exMessage, unsigned int _exID)
        : DeviceBackendException(_exMessage, _exID) {
      }
  };

} // namespace ChimeraTK

// backward compatibility
namespace mtca4u{
  using namespace ChimeraTK;
}

#endif /* CHIMERATK_REBOT_BACKEND_EXCEPTION_H */
