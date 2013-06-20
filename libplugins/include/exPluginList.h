/* 
 * File:   expluginManager.h
 * Author: apiotro
 *
 * Created on 1 sierpień 2012, 18:46
 */

#ifndef EXPLUGINLIST_H
#define	EXPLUGINLIST_H

#include "exBase.h"

class exPluginList : public exBase {
public:
    enum {EX_DUPLICATED_PLUGIN, EX_NO_PLUGIN_AVAILABLE};
    
    exPluginList(const std::string &_exMessage, unsigned int _exID);
    virtual ~exPluginList() throw();
private:

};

#endif	/* EXPLUGINLIST_H */

