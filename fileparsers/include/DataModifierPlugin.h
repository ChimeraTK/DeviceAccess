/*
 * DataModifierPlugin.h
 *
 *  Created on: Apr 7, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_DATA_MODIFIER_PLUGIN_H
#define MTCA4U_DATA_MODIFIER_PLUGIN_H

#include "RegisterInfoPlugin.h"
#include "VirtualFunctionTemplate.h"

namespace mtca4u {

  /** Base class for RegisterInfoPlugins which manipulate the data content of a register. Those plugins are no longer
   *  mere RegisterInfoPlugins but they still can provide additional information about the register. */
  class DataModifierPlugin : public RegisterInfoPlugin {

    public:

      /** Called by the backend when obtaining a buffering register accessor. This allows the plugin to decorate the
       *  accessor to change the its behaviour. */
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > decorateRegisterAccessor(
          boost::shared_ptr< NDRegisterAccessor<UserType> > accessor ) const {
        return CALL_VIRTUAL_FUNCTION_TEMPLATE(decorateRegisterAccessor_impl, UserType, accessor);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl,
          boost::shared_ptr< NDRegisterAccessor<T> >(boost::shared_ptr< NDRegisterAccessor<T> >) );

  };

} /* namespace mtca4u */


#endif /* MTCA4U_DATA_MODIFIER_PLUGIN_H */
