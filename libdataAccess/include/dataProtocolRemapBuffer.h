/* 
 * File:   dataProtocolBuffer.h
 * Author: apiotro
 *
 * Created on 28 styczeń 2012, 21:01
 */

#ifndef DATAPROTOCOLREMAPBUFFER_H
#define	DATAPROTOCOLREMAPBUFFER_H

#include "dataProtocol.h"
#include "dataProtocolElemRemapBuffer.h"
#include "singleton.h"
#include "libmap.h"
#include <map>

typedef SingletonHolder<dmapFilesParser, CreateByNew, LifetimeStandard, SingleThread> dmapFilesParserSingleton;

class dataProtocolRemapBuffer : public dataProtocol {
private:
    dmapFilesParser&                    dmapFiles;
    std::map<std::string, rawData*>     buffers;
public:
    dataProtocolRemapBuffer();
    virtual ~dataProtocolRemapBuffer();
    virtual dataProtocolElemRemapBuffer* createProtocolElem(const std::string& address);
    virtual void readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData); 
    void addBuffer(const std::string& bufName, rawData* buff);
    virtual void combine(dataProtocol* pD);
#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif

};

#endif	/* DATAPROTOCOLREMAPBUFFER_H */

