#include "dataAccess.h"
#include "exDataAccess.h"
#include "libmap.h"
#include "singleton.h"
#include "libdev.h"
#include "sstream"
#include "assert.h"
#include <iostream>

typedef SingletonHolder<dmapFilesParser, CreateByNew, LifetimeStandard, SingleThread> dmapFilesParserSingleton;

dataAccess::dataAccess()
{

}

dataAccess::~dataAccess() {

}

void dataAccess::init(const std::string& lmapFileName) {
    logicNameMapper::iterator iter;
    dataProtocol *pDp;
    std::string protocolName;
    std::string address;

    lnm.parse(lmapFileName);
    for (iter = lnm.begin(); iter != lnm.end(); ++iter) {
        protocolName = (*iter)->getProtocolName();
        address = (*iter)->getAddress();
        pDp = dpm.getProtocolObject(protocolName);
        (*iter)->setDataProtocolElem(pDp->createProtocolElem(address));
    }
}

void dataAccess::addProtocol(dataProtocol* pDp) {
    dpm.registerProtocol(pDp, &lnm);
}

void dataAccess::readData(const std::string& logName, rawData& data) {
    logicNameData* lnd = lnm.get(logName);
    dataProtocolElem* dpe = lnd->getDataProtocolElem();
    if (!dpe) {
        std::string address = lnd->getAddress();
        std::string protocolName = lnd->getProtocolName();
        dataProtocol *pDp = dpm.getProtocolObject(protocolName);
        dpe = pDp->createProtocolElem(address);
        if (!dpe) {
            throw exDataAccess("Incorrect configuration of logical name " + logName, exDataAccess::EX_REGISTER_NOT_INITILIZED_CORRECTLY);
        }
        lnd->setDataProtocolElem(dpe);
    }
    dpe->readData(data);
#ifdef __DEBUG_MODE__
    std::cout << "READ: " << *lnd;
#endif
}

void dataAccess::writeData(const std::string& logName, const rawData& data) {
    logicNameData* lnd = lnm.get(logName);
    dataProtocolElem* dpe = lnd->getDataProtocolElem();
    if (!dpe) {
        std::string address = lnd->getAddress();
        std::string protocolName = lnd->getProtocolName();
        dataProtocol *pDp = dpm.getProtocolObject(protocolName);
        dpe = pDp->createProtocolElem(address);
        if (!dpe) {
            throw exDataAccess("Incorrect configuration of logical name " + logName, exDataAccess::EX_REGISTER_NOT_INITILIZED_CORRECTLY);
        }
        lnd->setDataProtocolElem(dpe);
    }
    dpe->writeData(data);
#ifdef __DEBUG_MODE__    
    std::cout << "WRITE:" << *lnd;
#endif    
}

dataProtocolElem* dataAccess::getDeviceObject(const std::string& logName) {
    logicNameData* lnd = lnm.get(logName);
    dataProtocolElem* dpe = lnd->getDataProtocolElem();
    if (!dpe) {
        std::string address = lnd->getAddress();
        std::string protocolName = lnd->getProtocolName();
        dataProtocol *pDp = dpm.getProtocolObject(protocolName);
        dpe = pDp->createProtocolElem(address);
        if (!dpe) {
            throw exDataAccess("Incorrect configuration of logical name " + logName, exDataAccess::EX_REGISTER_NOT_INITILIZED_CORRECTLY);
        }
        lnd->setDataProtocolElem(dpe);
    }
    return dpe;
}

void dataAccess::readMetaData(metaDataLevel level, const std::string& logName, const std::string& metaDataTag, metaData& mData) {
    if (level == PROTOCOL_LEVEL) {
        logicNameData* lnd = lnm.get(logName);
        std::string protocolName = lnd->getProtocolName();
        dataProtocol *pDp = dpm.getProtocolObject(protocolName);
        pDp->readMetaData(logName, metaDataTag, mData);        
    } else if (level == DATA_CHANNEL_LEVEL) {
        logicNameData* lnd = lnm.get(logName);
        dataProtocolElem* dpe = lnd->getDataProtocolElem();
        if (!dpe) {
            std::string address = lnd->getAddress();
            std::string protocolName = lnd->getProtocolName();
            dataProtocol *pDp = dpm.getProtocolObject(protocolName);
            dpe = pDp->createProtocolElem(address);
            if (!dpe) {
                throw exDataAccess("Incorrect configuration of logical name " + logName, exDataAccess::EX_REGISTER_NOT_INITILIZED_CORRECTLY);
            }
            lnd->setDataProtocolElem(dpe);
        }
        dpe->readMetaData(metaDataTag, mData);
#ifdef __DEBUG_MODE__    
        std::cout << "READ META :" << *lnd;
#endif 
    } else {
        throw exDataAccess("Unknown typo of metadata", exDataAccess::EX_UNKNOWN_TYPE_OF_METADATA);
    }
}

#ifdef __DEBUG_MODE__    

std::ostream& operator<<(std::ostream& os, const dataAccess& da) {
    os << "Data Protocol Manager: " << std::endl << da.dpm << std::endl;
    os << "Logic Name Mapper: [" << da.lnm.getMapFileName() << "] " << std::endl << da.lnm << std::endl;
    return os;
}
#endif 

