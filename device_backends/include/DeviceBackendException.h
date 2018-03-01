/*
 * DeviceBackendException.h
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_DEVICE_BACKEND_EXCEPTION_H
#define CHIMERA_TK_DEVICE_BACKEND_EXCEPTION_H

#include "Exception.h"

namespace ChimeraTK {

  /** Exception class for all device backends to inherit.
   */
  class DeviceBackendException : public Exception {
    public:
      enum { EX_WRONG_PARAMETER
      };

      DeviceBackendException(const std::string &message, unsigned int exceptionID)
    : Exception( message, exceptionID ){};
  };

}

#endif /* CHIMERA_TK_DEVICE_BACKEND_EXCEPTION_H */
