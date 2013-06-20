#ifndef EXDATAACCESS_H
#define	EXDATAACCESS_H

#include "exBase.h"

class exDataAccess : public exBase { 
    public:
        enum {EX_INTERNAL_ERROR, EX_REGISTER_NOT_INITILIZED_CORRECTLY, EX_UNKNOWN_TYPE_OF_METADATA};
public:
    exDataAccess(const std::string &_exMessage, unsigned int _exID);
    virtual ~exDataAccess() throw();
private:

};

#endif	/* EXDATAACCESS_H */

