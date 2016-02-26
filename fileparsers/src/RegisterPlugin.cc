/*
 * RegisterPlugin.cc
 */

#include "RegisterPlugin.h"

namespace mtca4u {

  RegisterPlugin::~RegisterPlugin() {
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >* RegisterPlugin::getBufferingRegisterAccessorImpl(
      void *accessor_ptr) {
    auto accessor = static_cast<boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >*>(accessor_ptr);
    return accessor;
  }

  VIRTUAL_FUNCTION_TEMPLATE_IMPLEMENTER(RegisterPlugin, getBufferingRegisterAccessorImpl,
      getBufferingRegisterAccessorImpl, void*)

}
