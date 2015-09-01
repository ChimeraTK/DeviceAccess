#include "PcieDeviceException.h"

namespace mtca4u{

PcieDeviceException::PcieDeviceException(const std::string &_exMessage, unsigned int _exID)
 : DeviceException(_exMessage, _exID)
{
}

}//namespace mtca4u
