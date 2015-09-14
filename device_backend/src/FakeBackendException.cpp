#include "FakeBackendException.h"

namespace mtca4u{

FakeBackendException::FakeBackendException(const std::string &_exMessage, unsigned int _exID)
 : DeviceBackendException(_exMessage, _exID)
{
}

}//namespace mtca4u
