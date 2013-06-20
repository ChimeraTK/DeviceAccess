#ifndef DATAPROTOCOLELEMDOOCS_H
#define	DATAPROTOCOLELEMDOOCS_H

#include "dataProtocolElem.h"
#include "rawData.h"

class dataProtocolElemDOOCS : public dataProtocolElem {
public:
    dataProtocolElemDOOCS();
    virtual ~dataProtocolElemDOOCS();
    
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif    
    virtual void readData(rawData& data);
    virtual void writeData(const rawData& data);
    virtual void readMetaData(const std::string& metaDataTag, metaData& mData);
    virtual size_t getDataSize();
};

#endif	/* DATAPROTOCOLELEMDOOCS_H */

