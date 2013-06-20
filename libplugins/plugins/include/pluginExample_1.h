/* 
 * File:   pluginExample.h
 * Author: apiotro
 *
 * Created on 2 sierpień 2012, 08:57
 */

#ifndef PLUGINEXAMPLE_1_H
#define	PLUGINEXAMPLE_1_H

#include "pluginBase.h"

#define PLUGIN_EX_1_ID       7

class pluginExample_1 : pluginBase {
public:
    pluginExample_1();
    virtual ~pluginExample_1();
    void doSomething();    
private:

};

extern "C" {
pluginExample_1* create();
void destroy(pluginExample_1* obj);
int getPluginID();
}

#endif	/* PLUGINEXAMPLE_1_H */

