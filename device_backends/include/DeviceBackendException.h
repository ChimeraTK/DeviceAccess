/*
 * DeviceBackendException.h
 *
 *  Created on: Feb 1, 2016
 *      Author: mhier
 */

#ifndef DEVICEBACKENDEXCEPTION_H_
#define DEVICEBACKENDEXCEPTION_H_

#include "Exception.h"

namespace mtca4u {

  /** Exception class for all device backends to inherit.
   *
   */
  class DeviceBackendException : public Exception {
    public:
      enum { EX_WRONG_PARAMETER
      };

      DeviceBackendException(const std::string &message, unsigned int exceptionID)
    : Exception( message, exceptionID ){};
  };

}

#endif /* DEVICEBACKENDEXCEPTION_H_ */
