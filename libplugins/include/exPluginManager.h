/* 
 * File:   expluginManager.h
 * Author: apiotro
 *
 * Created on 1 sierpień 2012, 18:46
 */

#ifndef EXPLUGINMANAGER_H
#define	EXPLUGINMANAGER_H

#include "exBase.h"

class exPluginManager : public exBase {
public:
    enum {EX_NO_PLUGIN_DIRECTORY};
    
    exPluginManager(const std::string &_exMessage, unsigned int _exID);
    virtual ~exPluginManager() throw();
private:

};

#endif	/* EXPLUGINMANAGER_H */

