/*
 * RegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PLUGIN_H
#define MTCA4U_REGISTER_PLUGIN_H

#include "VirtualFunctionTemplate.h"
#include "SupportedUserTypes.h"

namespace mtca4u {

  // forward declaration
  template<typename UserType>
  class BufferingRegisterAccessorImpl;
  
  class RegisterAccessor;

  /** Base class for plugins providing modifications to registers and accessors. */
  class RegisterPlugin {
  
    public:

      RegisterPlugin() {
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateBufferingRegisterAccessor_impl);
      }
    
      /** Virtual destructor for a virtual base class */
      virtual ~RegisterPlugin();
      
      /** Called by the backend when obtaining a buffering register accessor. This allows the plugin to decorate the
       *  accessor to change the its behaviour. The default implementation just returns the passed accessor. */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > decorateBufferingRegisterAccessor(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor ) const {
        return CALL_VIRTUAL_FUNCTION_TEMPLATE(decorateBufferingRegisterAccessor_impl, UserType, accessor);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(decorateBufferingRegisterAccessor_impl,
          boost::shared_ptr< BufferingRegisterAccessorImpl<T> >(boost::shared_ptr< BufferingRegisterAccessorImpl<T> >) );

      /** Called by the backend when obtaining a non-buffering register accessor. This allows the plugin to decorate the
       *  accessor to change the its behaviour. The default implementation just returns the passed accessor. */
      virtual boost::shared_ptr<RegisterAccessor> decorateRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor) const;

    protected:

      /** Default implementation of getBufferingRegisterAccessor(): just return the unmodified accessor */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > decorateBufferingRegisterAccessor_impl(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const;
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(RegisterPlugin, decorateBufferingRegisterAccessor_impl, 1);
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
