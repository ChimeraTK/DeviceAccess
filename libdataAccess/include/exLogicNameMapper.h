#ifndef EXLOGICNAMEMAPPER_H
#define	EXLOGICNAMEMAPPER_H

#include "exBase.h"
#include <exception>
#include <string>



class exLogicNameMapper : public exBase {
public:
    enum {EX_FILE_NOT_FOUND, EX_ERROR_IN_FILE, EX_UNKNOWN_LOG_NAME};
public:
    exLogicNameMapper(const std::string &_exMessage, unsigned int _exID);
    virtual ~exLogicNameMapper() throw();


};

#endif	/* EXLOGICNAMEMAPPER_H */

