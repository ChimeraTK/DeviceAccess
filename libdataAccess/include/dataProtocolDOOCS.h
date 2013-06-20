#ifndef DATAPROTOCOLDOOCS_H
#define	DATAPROTOCOLDOOCS_H

#include "dataProtocol.h"
#include "dataProtocolElemDOOCS.h"

class dataProtocolDOOCS : public dataProtocol {
public:
    dataProtocolDOOCS();
    virtual ~dataProtocolDOOCS();
    virtual dataProtocolElemDOOCS* createProtocolElem(const std::string& address);
    virtual void readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData); 
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif
};

#endif	/* DATAPROTOCOLDOOCS_H */

