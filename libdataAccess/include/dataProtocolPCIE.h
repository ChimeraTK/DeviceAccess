#ifndef DATAPROTOCOLPCIE_H
#define	DATAPROTOCOLPCIE_H

#include "dataProtocol.h"
#include "libmap.h"
#include "libdev.h"
#include "dataProtocolElemPCIE.h"
#include "singleton.h"
#include <map>

typedef SingletonHolder<dmapFilesParser, CreateByNew, LifetimeStandard, SingleThread>   dmapFilesParserSingleton;


class dataProtocolPCIE : public dataProtocol {    
private:       
    dmapFilesParser&                    dmapFiles;
    std::map<std::string, __DEV__*>     hwAccess;   
public:
    dataProtocolPCIE(const std::string& dmapFile);
    virtual ~dataProtocolPCIE();
    virtual void combine(dataProtocol* pDp);    
    virtual dataProtocolElemPCIE* createProtocolElem(const std::string& address);
    virtual void readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData); 
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif

};

#endif	/* DATAPROTOCOLPCIE_H */

