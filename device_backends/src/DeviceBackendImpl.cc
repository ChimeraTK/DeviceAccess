/*
 * DeviceBackendImpl.cc
 */

#include "DeviceBackendImpl.h"
#include "BackendBufferingRegisterAccessor.h"

namespace mtca4u {

  DeviceBackendImpl::DeviceBackendImpl()
  : _opened(false), _connected(true) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  DeviceBackendImpl::~DeviceBackendImpl() {
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > DeviceBackendImpl::getBufferingRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t wordOffsetInRegister, size_t numberOfWords, bool enforceRawAccess) {
    auto accessor = boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >(
        new BackendBufferingRegisterAccessor<UserType>(shared_from_this(), registerPathName, wordOffsetInRegister,
            numberOfWords, enforceRawAccess) );
    // allow plugins to decorate the accessor and return it
    return decorateBufferingRegisterAccessor(registerPathName, accessor);
  }

} /* namespace mtca4u */
