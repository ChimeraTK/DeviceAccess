/* 
 * File:   dataProtocolElemAlias.cpp
 * Author: apiotro
 * 
 * Created on 3 luty 2012, 14:22
 */

#include "dataProtocolElemAlias.h"

dataProtocolElemAlias::dataProtocolElemAlias(dataProtocolElem*   _dpe) 
 : dataProtocolElem("ALIAS@"), dpe(_dpe){
}

dataProtocolElemAlias::~dataProtocolElemAlias() {
}

void dataProtocolElemAlias::readData(rawData& data)
{
    dpe->readData(data);
}

void dataProtocolElemAlias::writeData(const rawData& data) 
{
    dpe->writeData(data);
}

void dataProtocolElemAlias::readMetaData(const std::string& metaDataTag, metaData& mData)
{
    dpe->readMetaData(metaDataTag, mData);
}

size_t dataProtocolElemAlias::getDataSize()
{
    return dpe->getDataSize();    
}

#ifdef __DEBUG_MODE__     
std::ostream& dataProtocolElemAlias::show(std::ostream &os)
{
    os << " {[ALIAS]";
    dpe->show(os);
    os << "}";
    return os;
}
#endif 
