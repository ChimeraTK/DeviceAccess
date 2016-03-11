/*
 * RegisterPlugin.cc
 */

#include "RegisterPlugin.h"

namespace mtca4u {

  RegisterPlugin::~RegisterPlugin() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > RegisterPlugin::getBufferingRegisterAccessor_impl(
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const {
    return accessor;
  }

}
