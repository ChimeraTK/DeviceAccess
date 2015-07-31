#ifndef MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
#define MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H

#include "ExcBase.h"

namespace mtca4u{

  class NotImplementedException: public ExcBase {
  public:

    NotImplementedException(const std::string & message):
      ExcBase(message, 0) {}
    virtual ~NotImplementedException() throw(){}
  };

}// namespace mtca4u

#endif// MTCA4U_NOT_IMPLEMENTED_EXCEPTION_H
