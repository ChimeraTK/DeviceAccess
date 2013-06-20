#ifndef EXDATAPROTOCOLMANAGER_H
#define	EXDATAPROTOCOLMANAGER_H

#include "exBase.h"

class exDataProtocolManager  : public exBase {
public:
    enum {EX_PROTOCOL_ALREADY_REGISTERED, EX_UNKNOWN_PROTOCOL};
public:
    exDataProtocolManager(const std::string &_exMessage, unsigned int _exID);
    virtual ~exDataProtocolManager() throw();
private:

};

#endif	/* EXDATAPROTOCOLMANAGER_H */

