#include "PcieBackendException.h"

namespace mtca4u {

  PcieBackendException::PcieBackendException(const std::string &_exMessage, unsigned int _exID)
  : DeviceBackendException(_exMessage, _exID)
  {
  }

}//namespace mtca4u
