/* 
 * File:   exdevMap.cpp
 * Author: apiotro
 * 
 * Created on 11 marzec 2012, 20:42
 */

#include "exdevMap.h"

namespace mtca4u{

exdevMap::exdevMap(const std::string &_exMessage, unsigned int _exID)
: exBase(_exMessage, _exID) {
}

exdevMap::~exdevMap() throw() {
}

}//namespace mtca4u
