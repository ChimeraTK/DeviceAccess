/* 
 * File:   exdevMap.cpp
 * Author: apiotro
 * 
 * Created on 11 marzec 2012, 20:42
 */

#include "DeviceException.h"

namespace mtca4u{

DeviceException::DeviceException(const std::string &_exMessage, unsigned int _exID)
: DeviceBackendException(_exMessage, _exID) {
}

DeviceException::~DeviceException() throw() {
}

}//namespace mtca4u
