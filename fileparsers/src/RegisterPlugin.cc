/*
 * RegisterPlugin.cc
 */

#include <boost/smart_ptr.hpp>

#include "RegisterPlugin.h"
#include "NDRegisterAccessor.h"

namespace mtca4u {

  RegisterPlugin::~RegisterPlugin() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > RegisterPlugin::decorateRegisterAccessor_impl(
      boost::shared_ptr< NDRegisterAccessor<UserType> > accessor) const {
    return accessor;
  }

}
