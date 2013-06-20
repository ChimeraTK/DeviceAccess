/* 
 * File:   expluginManager.cpp
 * Author: apiotro
 * 
 * Created on 1 sierpień 2012, 18:46
 */

#include "exPluginManager.h"

exPluginManager::exPluginManager(const std::string &_exMessage, unsigned int _exID)
: exBase(_exMessage, _exID) {
}



exPluginManager::~exPluginManager() throw() {
}

