/* 
 * File:   exdevMap.cpp
 * Author: apiotro
 * 
 * Created on 11 marzec 2012, 20:42
 */

#include "ExcMappedDevice.h"

namespace mtca4u{

ExcMappedDevice::ExcMappedDevice(const std::string &_exMessage, unsigned int _exID)
: Exception(_exMessage, _exID) {
}

ExcMappedDevice::~ExcMappedDevice() throw() {
}

}//namespace mtca4u
