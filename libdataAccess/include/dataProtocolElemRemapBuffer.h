/* 
 * File:   dataProtocolElemBuffer.h
 * Author: apiotro
 *
 * Created on 28 styczeń 2012, 21:02
 */

#ifndef DATAPROTOCOLELEMBUFFER_H
#define	DATAPROTOCOLELEMBUFFER_H

#include "dataProtocolElem.h"
#include <string>

class dataProtocolElemRemapBuffer : public dataProtocolElem {
private:
    unsigned int regTotalOffset;
    unsigned int regTotalSize;
    rawData*     buff;    
public:
    dataProtocolElemRemapBuffer(unsigned int _regTotalOffset, unsigned int _regTotalSize, rawData* _buff, const std::string &address);
    virtual ~dataProtocolElemRemapBuffer();

    virtual void readData(rawData& data);
    virtual void writeData(const rawData& data);
    virtual void readMetaData(const std::string& metaDataTag, metaData& mData);
    virtual size_t getDataSize();

#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os);
#endif 

};

#endif	/* DATAPROTOCOLELEMBUFFER_H */

