#ifndef MTCA4U_SEQUENCE_DE_MULTIPLEXER_EXCEPTION_H
#define	MTCA4U_SEQUENCE_DE_MULTIPLEXER_EXCEPTION_H

#include "exBase.h"

namespace mtca4u{

class MultiplexedDataAccessorException : public exBase {
public:
    
  enum { EMPTY_AREA, INVALID_WORD_SIZE, INVALID_N_ELEMENTS };
    
    
  MultiplexedDataAccessorException(const std::string &message, unsigned int ID)
    : exBase(message, ID){}
};

}//namespace mtca4u

#endif	/* MTCA4U_SEQUENCE_DE_MULTIPLEXER_EXCEPTION_H */

