/* 
 * File:   dataProtocolAlias.cpp
 * Author: apiotro
 * 
 * Created on 3 luty 2012, 14:22
 */

#include "dataProtocolAlias.h"
#include "dataProtocolElemAlias.h"
#include "logicNameMapper.h"
#include "exDataProtocol.h"

dataProtocolAlias::dataProtocolAlias() 
: dataProtocol("ALIAS")
{
}

dataProtocolAlias::~dataProtocolAlias() {
}

dataProtocolElem* dataProtocolAlias::createProtocolElem(const std::string& address)
{
    logicNameData* lnd;
    dataProtocolElem*    dpe;
    
    lnd = lnm->get(address);    
    dpe = lnd->getDataProtocolElem();    
    if (!dpe){
        return NULL;
    }                
    return new dataProtocolElemAlias(dpe);
}

void dataProtocolAlias::readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData)
{
    throw exDataProtocol("Tag \"" + metaDataTag + "\" not supported for \"" + protName + "\" protocol" , exDataProtocol::EX_NOT_SUPPORTED);
}

#ifdef __DEBUG_MODE__     
std::ostream& dataProtocolAlias::show(std::ostream &os)
{
    os << "dataProtocolAlias: " << std::endl;
    os << "<<EMPTY>>";
    return os;
}
#endif
