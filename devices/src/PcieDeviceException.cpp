#include "ExceptionPcieDevice.h"

namespace mtca4u{

PcieDeviceException::PcieDeviceException(const std::string &_exMessage, unsigned int _exID)
 : Exception(_exMessage, _exID)
{
}

}//namespace mtca4u
