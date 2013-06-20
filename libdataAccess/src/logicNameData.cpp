#include "logicNameData.h"
#include <iomanip>
#include <iostream>

logicNameData::logicNameData()
 : dataProtocol(NULL)
{
}

logicNameData::logicNameData(const std::string &_logName, const std::string &_protName, const std::string &_address, unsigned int _lineNr) 
   : logName(_logName), protName(_protName), address(_address), lineNr(_lineNr), dataProtocol(NULL)
{
}


logicNameData::~logicNameData() {
    delete dataProtocol;
}

dataProtocolElem* logicNameData::getDataProtocolElem()
{
    return dataProtocol;
}

std::string logicNameData::getProtocolName() const
{
    return protName;
}

std::string logicNameData::getAddress() const
{
    return address;
}

void logicNameData::setDataProtocolElem(dataProtocolElem *_dataProtocolElem)
{    
    dataProtocol = _dataProtocolElem;
}

#ifdef __DEBUG_MODE__
std::ostream& operator<<(std::ostream& os, const logicNameData &lnd)
{
    os << std::setw(10) << lnd.logName << " -> " <<lnd.protName << ":" << lnd.address;      
    if (lnd.dataProtocol){
        lnd.dataProtocol->show(os);
    } else {
        os << " {[NULL]}";
    }
    os << std::endl;
    return os;
}
#endif


