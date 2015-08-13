#include "ExcFakeDevice.h"

namespace mtca4u{

ExcFakeDevice::ExcFakeDevice(const std::string &_exMessage, unsigned int _exID)
 : Exception(_exMessage, _exID)
{
}

}//namespace mtca4u
