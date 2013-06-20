/* 
 * File:   dataProtocolElem.h
 * Author: apiotro
 *
 */

#ifndef DATAPROTOCOLELEM_H
#define	DATAPROTOCOLELEM_H

#include "rawData.h"
#include "metaData.h"
#include <string>
#include <ostream>

class dataProtocolElem {
private:
    std::string address;
public:
    std::string getAddress();    
    dataProtocolElem(const std::string &_address);        
    virtual ~dataProtocolElem();        
    virtual void readData(rawData& data)  = 0;
    virtual void writeData(const rawData& data) = 0;    
    virtual void readMetaData(const std::string& metaDataTag, metaData& mData) = 0;
    virtual size_t getDataSize() = 0;    
    
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os) = 0;
#endif     
private:

};

#endif	/* DATAPROTOCOLELEM_H */

