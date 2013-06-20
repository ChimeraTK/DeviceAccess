#ifndef DATAPROTOCOLELEMPCIE_H
#define	DATAPROTOCOLELEMPCIE_H

#include "dataProtocolElem.h"
#include "libmap.h"
#include "libdev.h"
#include "rawData.h"
#include <string>

#ifdef __USE_PCIE_FAKE_DEV__
#include "devFake.h"  
#define __DEV__ devFake
#else
#include "devPCIE.h"
#define __DEV__ devPCIE
#endif

class dataProtocolElemPCIE : public dataProtocolElem {
 private:
    const std::string   devName;
    const std::string   regName;
    unsigned int        regInternalOffset;
    unsigned int        regInternalSize;        
    std::string         devFileName;
    mapFile::mapElem    elem;
    
    unsigned int        totalRegOffset;
    unsigned int        totalRegSize;
    
    __DEV__             *dp;
    
public:
    dataProtocolElemPCIE(const std::string &_devName, const std::string &_regName, unsigned int _regInternalOffset, unsigned int _regInternalSize, const std::string &devName, mapFile::mapElem& _elem, __DEV__ *_dp, unsigned int _totalRegOffset, unsigned int _totalRegSize);
    virtual ~dataProtocolElemPCIE();
    
    
    virtual void readData(rawData &data);
    virtual void writeData(const rawData &data);
    virtual void readMetaData(const std::string& metaDataTag, metaData& mData);
    virtual size_t getDataSize();
    
#ifdef __DEBUG_MODE__    
    virtual std::ostream& show(std::ostream &os);
#endif

};

#endif	/* DATAPROTOCOLELEMPCIE_H */

