/*
 * RegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PLUGIN_H
#define MTCA4U_REGISTER_PLUGIN_H

#include "ForwardDeclarations.h"
#include "VirtualFunctionTemplate.h"
#include "SupportedUserTypes.h"

namespace mtca4u {

  /** Base class for plugins providing modifications to registers and accessors. */
  class RegisterInfoPlugin {
  
    public:

      RegisterInfoPlugin() {
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl);
      }
    
      /** Virtual destructor for a virtual base class */
      virtual ~RegisterInfoPlugin();
      
      /** Called by the backend when obtaining a buffering register accessor. This allows the plugin to decorate the
       *  accessor to change the its behaviour. The default implementation just returns the passed accessor. */
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > decorateRegisterAccessor(
          boost::shared_ptr< NDRegisterAccessor<UserType> > accessor ) const {
        return CALL_VIRTUAL_FUNCTION_TEMPLATE(decorateRegisterAccessor_impl, UserType, accessor);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateRegisterAccessor_impl,
          boost::shared_ptr< NDRegisterAccessor<T> >(boost::shared_ptr< NDRegisterAccessor<T> >) );

    protected:

      /** Default implementation of getBufferingRegisterAccessor(): just return the unmodified accessor */
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > decorateRegisterAccessor_impl(
          boost::shared_ptr< NDRegisterAccessor<UserType> > accessor) const;
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(RegisterInfoPlugin, decorateRegisterAccessor_impl, 1);
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
