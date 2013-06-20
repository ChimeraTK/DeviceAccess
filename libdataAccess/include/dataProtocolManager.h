#ifndef DATAPROTOCOLMANAGER_H
#define	DATAPROTOCOLMANAGER_H

#include "dataProtocol.h"
#include "logicNameMapper.h"
#include <vector>
#include <string>

class dataProtocolManager {
private:
    class namePredicate{
    private:
        std::string name;
    public:
        namePredicate(const std::string& _name);
        bool operator()(dataProtocol* pDp);
    };
private:
    std::vector<dataProtocol*> dataProtocols;
    
public:
    dataProtocolManager();
    virtual ~dataProtocolManager();
    
    void          registerProtocol(dataProtocol* pDp, const logicNameMapper* _lnm);
    dataProtocol* getProtocolObject(const std::string& protName);
    
#ifdef __DEBUG_MODE__    
    friend std::ostream& operator<<(std::ostream& os, const dataProtocolManager& da);
 #endif 
};

#endif	/* DATAPROTOCOLMANAGER_H */

