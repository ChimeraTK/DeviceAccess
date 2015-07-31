#ifndef MTCA4U_SEQUENCE_DE_MULTIPLEXER_EXCEPTION_H
#define	MTCA4U_SEQUENCE_DE_MULTIPLEXER_EXCEPTION_H

#include "ExcBase.h"

namespace mtca4u{

class MultiplexedDataAccessorException : public ExcBase {
public:
    
  enum { EMPTY_AREA, INVALID_WORD_SIZE, INVALID_N_ELEMENTS };
    
    
  MultiplexedDataAccessorException(const std::string &message, unsigned int ID)
    : ExcBase(message, ID){}
};

}//namespace mtca4u

#endif	/* MTCA4U_SEQUENCE_DE_MULTIPLEXER_EXCEPTION_H */

