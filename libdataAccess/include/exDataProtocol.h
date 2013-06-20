#ifndef EXDATAPROTOCOL_H
#define	EXDATAPROTOCOL_H

#include "exBase.h"

class exDataProtocol : public exBase {
public:
    enum {EX_UNKNOWN_METADATA_TAG, EX_PROTOCOL_ALREADY_REGISTERED, EX_WRONG_ADDRESS, EX_INTERNAL_ERROR, EX_WRONG_BUFFER_SIZE, EX_NOT_SUPPORTED, EX_BUFFER_ALREADY_REGISTERED, EX_BUFFER_NOT_INITIALIZED};
public:
    exDataProtocol(const std::string &_exMessage, unsigned int _exID);
    virtual ~exDataProtocol() throw();
private:

};

#endif	/* EXDATAPROTOCOL_H */

