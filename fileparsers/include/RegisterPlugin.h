/*
 * RegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PLUGIN_H
#define MTCA4U_REGISTER_PLUGIN_H

#include "Value.h"
#include "SupportedUserTypes.h"

namespace mtca4u {

  /** Base class for plugins providing modifications to registers and accessors. */
  class RegisterPlugin {
  
    public:

      RegisterPlugin() {
        FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl);
      }
    
      /** Virtual destructor for a virtual base class */
      virtual ~RegisterPlugin();
      
      /** Called by the backend when obtaining a buffering register accessor. This allows the plugin to put some
       *  intermediate accessor in between to change the accessor's behaviour. The default implementation just returns
       *  the passed accessor. */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor ) {
        return CALL_VIRTUAL_FUNCTION_TEMPLATE(getBufferingRegisterAccessor_impl, UserType, accessor);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl,
          boost::shared_ptr< BufferingRegisterAccessorImpl<T> >(boost::shared_ptr< BufferingRegisterAccessorImpl<T> >) );

      /** Called by the backend when obtaining a (non-buffering) register accessor. This allows the plugin to put some
       *  intermediate accessor in between to change the accessor's behaviour. The default implementation just returns
       *  the passed accessor. */
      virtual boost::shared_ptr<RegisterAccessor> getRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor) {
        return accessor;
      }

    protected:

      /** Default implementation of getBufferingRegisterAccessor(): just return the unmodified accessor */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor_impl(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const;
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(RegisterPlugin, getBufferingRegisterAccessor_impl, 1);
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
