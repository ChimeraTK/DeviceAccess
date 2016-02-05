/*
 * RebotBackendException.h
 *
 *  Created on: May 29, 2015
 *      Author: adagio
 */

#ifndef MTCA4U_REBOT_BACKEND_EXCEPTION_H
#define MTCA4U_REBOT_BACKEND_EXCEPTION_H

#include <DeviceBackendException.h>
#include <string>

namespace mtca4u {

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
      RebotBackendException(const std::string &_exMessage, unsigned int _exID);
  };

} // namespace mtca4u

#endif /* MTCA4U_REBOT_BACKEND_EXCEPTION_H */
