#ifndef CHIMERA_TK_DEVICE_EXCEPTION_H
#define CHIMERA_TK_DEVICE_EXCEPTION_H

#include "DeviceBackendException.h"
#warning You are using a depcrecated header file. Please switch to Exception.h and use the ChimeraTK::runtime_error instead of the old Exception names.

namespace ChimeraTK {

/**
 *  Compatibility typedefs for the old exception classes
 */
typedef ChimeraTK::runtime_error DeviceException;
typedef DeviceException TwoDRegisterAccessorException;
typedef DeviceException MultiplexedDataAccessorException;

} // namespace ChimeraTK

#endif /* CHIMERA_TK_DEVICE_EXCEPTION_H */
