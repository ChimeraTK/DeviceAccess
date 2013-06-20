/* 
 * File:   pluginExample.cpp
 * Author: apiotro
 * 
 * Created on 2 sierpień 2012, 08:57
 */

#include "pluginExample_2.h"
#include <iostream>

pluginExample_2::pluginExample_2() {
}


pluginExample_2::~pluginExample_2() {
}

void pluginExample_2::doSomething()
{
    std::cout << "void pluginExample_2::doSomething()" << std::endl;
}

extern "C" {
    
pluginExample_2* create(){
    return new pluginExample_2();
}

void destroy(pluginExample_2* obj){
    delete obj;
}

int getPluginID(){
    return PLUGIN_EX_2_ID;
}

}