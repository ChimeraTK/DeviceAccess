/* 
 * File:   exdevMap.cpp
 * Author: apiotro
 * 
 * Created on 11 marzec 2012, 20:42
 */

#include "MappedDeviceException.h"

namespace mtca4u{

MappedDeviceException::MappedDeviceException(const std::string &_exMessage, unsigned int _exID)
: DeviceException(_exMessage, _exID) {
}

MappedDeviceException::~MappedDeviceException() throw() {
}

}//namespace mtca4u
