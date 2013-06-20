#include "dataProtocol.h"
#include "dbg_print.h"

dataProtocol::dataProtocol(const std::string& _protName)
: protName(_protName)
{
    
}

dataProtocol::~dataProtocol()
{
    
}

std::string dataProtocol::getProtocolName() const
{
    return protName;
}

void dataProtocol::setLogicNameMapper(const logicNameMapper* _lnm)
{
    lnm = _lnm;
}


void dataProtocol::combine(dataProtocol* pDp)
{
    dbg_print("%s\n", "combine - base class");
}

