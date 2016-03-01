/*
 * DeviceBackend.cc
 */

#include "DeviceBackend.h"
#include "BackendBufferingRegisterAccessor.h"

namespace mtca4u {

  DeviceBackend::DeviceBackend() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  DeviceBackend::~DeviceBackend() {
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > DeviceBackend::getBufferingRegisterAccessor_impl(
      const std::string &registerName, const std::string &module) {
    return boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >(
        new BackendBufferingRegisterAccessor<UserType>(shared_from_this(),registerName,module) );
  }

} /* namespace mtca4u */
