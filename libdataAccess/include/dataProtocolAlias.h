/* 
 * File:   dataProtocolAlias.h
 * Author: apiotro
 *
 * Created on 3 luty 2012, 14:22
 */

#ifndef DATAPROTOCOLALIAS_H
#define	DATAPROTOCOLALIAS_H

#include "dataProtocol.h"

class dataProtocolAlias  : public dataProtocol  {
public:
    dataProtocolAlias();
    virtual ~dataProtocolAlias();
    virtual dataProtocolElem* createProtocolElem(const std::string& address);
    virtual void readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData);        
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif
private:

};

#endif	/* DATAPROTOCOLALIAS_H */

