/*
 * DeviceBackendException.h
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_DEVICE_BACKEND_EXCEPTION_H
#define MTCA4U_DEVICE_BACKEND_EXCEPTION_H

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

#endif /* MTCA4U_DEVICE_BACKEND_EXCEPTION_H */
