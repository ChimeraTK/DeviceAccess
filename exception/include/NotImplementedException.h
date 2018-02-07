#ifndef MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
#define MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H

#include "Exception.h"

namespace ChimeraTK {

  class NotImplementedException: public Exception {
    public:

      NotImplementedException(const std::string & message):
        Exception(message, 0) {}
      virtual ~NotImplementedException() throw(){}      // LCOV_EXCL_LINE
  };

}// namespace ChimeraTK

#endif// MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
