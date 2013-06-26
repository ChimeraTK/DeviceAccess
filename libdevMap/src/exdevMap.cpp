/* 
 * File:   exdevMap.cpp
 * Author: apiotro
 * 
 * Created on 11 marzec 2012, 20:42
 */

#include "exdevMap.h"

exdevMap::exdevMap(const std::string &_exMessage, unsigned int _exID)
: exBase(_exMessage, _exID) {
}

exdevMap::~exdevMap() throw() {
}

//fixme: this does not to anything
std::ostream& operator<<(std::ostream &os, const exdevMap& /*e*/)
{
    return os;
}

