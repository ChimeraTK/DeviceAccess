#include "dataProtocolManager.h"
#include "exDataProtocolManager.h"
#include "dbg_print.h"
#include <algorithm>

dataProtocolManager::namePredicate::namePredicate(const std::string& _name)
 : name(_name)
{
    
}

bool dataProtocolManager::namePredicate::operator ()(dataProtocol* pDp)
{
    if (pDp->getProtocolName() == name) 
        return true;
    else
        return false;
}

dataProtocolManager::dataProtocolManager() {
}

dataProtocolManager::~dataProtocolManager() {
    std::vector<dataProtocol*>::iterator iter;
    for (iter = dataProtocols.begin(); iter != dataProtocols.end(); iter++){
        delete (*iter);
    }
}

void dataProtocolManager::registerProtocol(dataProtocol* pDp, const logicNameMapper* _lnm)
{
    std::vector<dataProtocol*>::iterator iter;
    iter = std::find_if(dataProtocols.begin(), dataProtocols.end(), namePredicate(pDp->getProtocolName()));
    if (iter != dataProtocols.end()){
        (*iter)->combine(pDp);
        delete pDp;
        return;
    }
    pDp->setLogicNameMapper(_lnm);
    dataProtocols.push_back(pDp);    
    return;
}

dataProtocol* dataProtocolManager::getProtocolObject(const std::string& protName)
{
    std::vector<dataProtocol*>::iterator iter;
    iter = std::find_if(dataProtocols.begin(), dataProtocols.end(), namePredicate(protName));
    if (iter == dataProtocols.end()){
	throw exDataProtocolManager("Unknown protocol: " + protName, exDataProtocolManager::EX_UNKNOWN_PROTOCOL);
    }
    return *iter;
}

#ifdef __DEBUG_MODE__    
std::ostream& operator<<(std::ostream& os, const dataProtocolManager& da)
{
    std::vector<dataProtocol*>::const_iterator iter;
    for (iter = da.dataProtocols.begin(); iter != da.dataProtocols.end(); ++iter){
        (*iter)->show(os);
    }
    return os;
}
 #endif 
