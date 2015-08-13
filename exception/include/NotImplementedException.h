#ifndef MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
#define MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H

#include "Exception.h"

namespace mtca4u{

  class NotImplementedException: public Exception {
  public:

    NotImplementedException(const std::string & message):
      Exception(message, 0) {}
    virtual ~NotImplementedException() throw(){}
  };

}// namespace mtca4u

#endif// MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
