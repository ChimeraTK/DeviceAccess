#ifndef CHIMERA_TK_NOT_IMPLEMENTED_EXCEPTION_H
#define CHIMERA_TK_NOT_IMPLEMENTED_EXCEPTION_H

#include "Exception.h"

namespace ChimeraTK {

  class NotImplementedException: public Exception {
    public:

      NotImplementedException(const std::string & message):
        Exception(message, 0) {}
      virtual ~NotImplementedException() throw(){}      // LCOV_EXCL_LINE
  };

}// namespace ChimeraTK

#endif// CHIMERA_TK_NOT_IMPLEMENTED_EXCEPTION_H
