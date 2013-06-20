/* 
 * File:   expluginManager.h
 * Author: apiotro
 *
 * Created on 1 sierpień 2012, 18:46
 */

#ifndef EXPLUGINELEM_H
#define	EXPLUGINELEM_H

#include "exBase.h"

class exPluginElem : public exBase {
public:
    enum {EX_CANNOT_OPEN_PLUGIN, EX_CANNOT_LOAD_FUNCTION};
    
    exPluginElem(const std::string &_exMessage, unsigned int _exID);
    virtual ~exPluginElem() throw();
private:

};

#endif	/* EXPLUGINELEM_H */

