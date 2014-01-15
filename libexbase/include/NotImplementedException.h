#ifndef MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
#define MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H

#include "exBase.h"

namespace mtca4u{

  class NotImplementedException: public exBase {
  public:

    NotImplementedException(const std::string & message):
      exBase(message, 0) {}
    virtual ~NotImplementedException() throw(){}
  };

}// namespace mtca4u

#endif// MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
