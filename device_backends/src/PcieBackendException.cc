#include "PcieBackendException.h"

namespace ChimeraTK {

  PcieBackendException::PcieBackendException(const std::string &_exMessage, unsigned int _exID)
  : DeviceBackendException(_exMessage, _exID)
  {
  }

}//namespace ChimeraTK
