#include "ExcPcieDevice.h"

namespace mtca4u{

ExcPcieDevice::ExcPcieDevice(const std::string &_exMessage, unsigned int _exID)
 : Exception(_exMessage, _exID)
{
}

}//namespace mtca4u
