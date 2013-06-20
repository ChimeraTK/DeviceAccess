/* 
 * File:   pluginExample.h
 * Author: apiotro
 *
 * Created on 2 sierpień 2012, 08:57
 */

#ifndef PLUGINEXAMPLE_2_H
#define	PLUGINEXAMPLE_2_H

#include "pluginBase.h"

#define PLUGIN_EX_2_ID       77

class pluginExample_2 : pluginBase {
public:
    pluginExample_2();
    virtual ~pluginExample_2();
    void doSomething(); 
private:

};

extern "C" {
pluginExample_2* create();
void destroy(pluginExample_2* obj);
int getPluginID();
}

#endif	/* PLUGINEXAMPLE_2_H */

