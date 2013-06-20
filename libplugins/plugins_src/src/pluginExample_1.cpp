/* 
 * File:   pluginExample.cpp
 * Author: apiotro
 * 
 * Created on 2 sierpień 2012, 08:57
 */

#include "pluginExample_1.h"
#include <iostream>

pluginExample_1::pluginExample_1() {
}


pluginExample_1::~pluginExample_1() {
}

void pluginExample_1::doSomething()
{
    std::cout << "void pluginExample_1::doSomething()" << std::endl;
}

extern "C" {
    
pluginExample_1* create(){
    return new pluginExample_1();
}

void destroy(pluginExample_1* obj){
    delete obj;
}

int getPluginID(){
    return PLUGIN_EX_1_ID;
}

}