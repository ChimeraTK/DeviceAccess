#include "FakeDeviceException.h"

namespace mtca4u{

FakeDeviceException::FakeDeviceException(const std::string &_exMessage, unsigned int _exID)
 : Exception(_exMessage, _exID)
{
}

}//namespace mtca4u
