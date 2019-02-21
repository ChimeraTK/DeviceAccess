#ifndef CHIMERA_TK_NOT_IMPLEMENTED_EXCEPTION_H
#define CHIMERA_TK_NOT_IMPLEMENTED_EXCEPTION_H

#include "Exception.h"
#warning You are using a depcrecated header file. Please switch to Exception.h and use the ChimeraTK::runtime_error instead of the old Exception names.

namespace ChimeraTK {

/**
 *  Compatibility typedefs for the old exception classes
 */
typedef ChimeraTK::runtime_error NotImplementedException;

} // namespace ChimeraTK

#endif // CHIMERA_TK_NOT_IMPLEMENTED_EXCEPTION_H
