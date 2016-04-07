/*
 * RegisterPlugin.cc
 */

#include <boost/smart_ptr.hpp>

#include "RegisterInfoPlugin.h"
#include "NDRegisterAccessor.h"

namespace mtca4u {

  RegisterInfoPlugin::~RegisterInfoPlugin() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > RegisterInfoPlugin::decorateRegisterAccessor_impl(
      boost::shared_ptr< NDRegisterAccessor<UserType> > accessor) const {
    return accessor;
  }

}
