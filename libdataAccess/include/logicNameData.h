#ifndef LOGICNAMEDATA_H
#define	LOGICNAMEDATA_H

#include <string>
#include <ostream>
#include "dataProtocol.h"
#include "dataProtocolElem.h"


class logicNameData {
private:
    std::string         logName;
    std::string         protName;
    std::string         address; 
    unsigned int        lineNr;
    dataProtocolElem    *dataProtocol;

    
    logicNameData(const logicNameData& elem);
public:
    logicNameData();
    logicNameData(const std::string &_logName, const std::string &_protName, const std::string &_address, unsigned int _lineNr);       
    
    std::string         getProtocolName() const;
    std::string         getAddress() const;
    std::string         getLogicName();
    dataProtocolElem   *getDataProtocolElem();
    void                setDataProtocolElem(dataProtocolElem *_dataProtocolElem);
    
    
    
    
    virtual ~logicNameData();
#ifdef __DEBUG_MODE__    
    friend std::ostream& operator<<(std::ostream& os, const logicNameData &lnd);
#endif    
private:

};

#endif	/* LOGICNAMEDATA_H */

