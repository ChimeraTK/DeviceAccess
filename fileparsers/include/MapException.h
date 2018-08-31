#ifndef CHIMERA_TK_MAP_EXCEPTION_H
#define CHIMERA_TK_MAP_EXCEPTION_H

#include <string>

#include "Exception.h"

#warning You are using a depcrecated header file. Please switch to Exception.h and use the ChimeraTK::runtime_error instead of the old Exception names.

namespace ChimeraTK {

  /**
   *  Compatibility typedefs for the old exception classes
   */
  typedef ChimeraTK::runtime_error LibMapException;
  typedef ChimeraTK::runtime_error MapFileException;
  typedef ChimeraTK::runtime_error MapFileParserException;
  typedef ChimeraTK::runtime_error DMapFileException;
  typedef ChimeraTK::runtime_error DMapFileParserException;

}//namespace ChimeraTK

#endif  /* CHIMERA_TK_MAP_EXCEPTION_H */

