/*
 * DeviceBackendException.h
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_DEVICE_BACKEND_EXCEPTION_H
#define CHIMERA_TK_DEVICE_BACKEND_EXCEPTION_H

#include "Exception.h"
#warning You are using a depcrecated header file. Please switch to Exception.h and use the ChimeraTK::runtime_error instead of the old Exception names.

namespace ChimeraTK {

  /**
   *  Compatibility typedefs for the old exception classes
   */
  typedef ChimeraTK::runtime_error DeviceBackendException;

} // namespace ChimeraTK

#endif /* CHIMERA_TK_DEVICE_BACKEND_EXCEPTION_H */
