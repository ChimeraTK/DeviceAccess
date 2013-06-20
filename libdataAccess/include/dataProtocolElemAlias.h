/* 
 * File:   dataProtocolElemAlias.h
 * Author: apiotro
 *
 * Created on 3 luty 2012, 14:22
 */

#ifndef DATAPROTOCOLELEMALIAS_H
#define	DATAPROTOCOLELEMALIAS_H

#include "dataProtocolElem.h"

class dataProtocolElemAlias : public dataProtocolElem {
private:
    dataProtocolElem* dpe;
public:
    dataProtocolElemAlias(dataProtocolElem* _dpe);
    virtual ~dataProtocolElemAlias();
    virtual void readData(rawData& data);
    virtual void writeData(const rawData& data);    
    virtual void readMetaData(const std::string& metaDataTag, metaData& mData);
    virtual size_t getDataSize();
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif     
private:

};

#endif	/* DATAPROTOCOLELEMALIAS_H */

