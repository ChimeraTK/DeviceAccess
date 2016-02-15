/*
 * DeviceBackend.cc
 */

#include "DeviceBackend.h"
#include "BackendBufferingRegisterAccessor.h"

namespace mtca4u {

  DeviceBackend::~DeviceBackend() {
  }

  /********************************************************************************************************************/

  template<typename UserType>
  BufferingRegisterAccessorImpl<UserType>* DeviceBackend::getBackendBufferingRegisterAccessor(
      const std::string &registerName, const std::string &module) {
    return static_cast< BufferingRegisterAccessorImpl<UserType>* >(
        new BackendBufferingRegisterAccessor<UserType>(shared_from_this(),registerName,module) );
  }

  VIRTUAL_FUNCTION_TEMPLATE_IMPLEMENTER(DeviceBackend, getBufferingRegisterAccessorImpl,
      getBackendBufferingRegisterAccessor, const std::string &, const std::string &)

}
