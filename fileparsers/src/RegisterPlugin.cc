/*
 * RegisterPlugin.cc
 */

#include <boost/smart_ptr.hpp>

#include "RegisterPlugin.h"
#include "BufferingRegisterAccessorImpl.h"

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
 
  /********************************************************************************************************************/

  boost::shared_ptr<RegisterAccessor> RegisterPlugin::getRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor) {
    return accessor;
  }

}
