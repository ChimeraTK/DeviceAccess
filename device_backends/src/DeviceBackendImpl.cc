/*
 * DeviceBackendImpl.cc
 */

#include "DeviceBackendImpl.h"

namespace mtca4u {

  DeviceBackendImpl::DeviceBackendImpl()
  : _opened(false), _connected(true)
  {}

  /********************************************************************************************************************/

  DeviceBackendImpl::~DeviceBackendImpl() {
  }

} /* namespace mtca4u */
