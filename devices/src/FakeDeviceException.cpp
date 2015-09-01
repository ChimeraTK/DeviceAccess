#include "FakeDeviceException.h"

namespace mtca4u{

FakeDeviceException::FakeDeviceException(const std::string &_exMessage, unsigned int _exID)
 : DeviceException(_exMessage, _exID)
{
}

}//namespace mtca4u
