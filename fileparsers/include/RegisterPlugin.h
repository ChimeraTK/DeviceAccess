/*
 * RegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PLUGIN_H
#define MTCA4U_REGISTER_PLUGIN_H

#include "Value.h"

namespace mtca4u {

  /** Base class for plugins providing modifications to registers and accessors. */
  class RegisterPlugin {
  
    public:
    
      /** Virtual destructor for a virtual base class */
      virtual ~RegisterPlugin();
      
      /** Called by the backend when obtaining a buffering register accessor. This allows the plugin to put some
       *  intermediate accessor in between to change the accessor's behaviour. The default implementation just returns
       *  the passed accessor. */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor );

      VIRTUAL_FUNCTION_TEMPLATE_DECLARATION(getBufferingRegisterAccessorImpl, void*);

    protected:

      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >* getBufferingRegisterAccessorImpl(void *accessor_ptr);
  
  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > RegisterPlugin::getBufferingRegisterAccessor(
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor)
  {
    auto ptr = VIRTUAL_FUNCTION_TEMPLATE_CALL(getBufferingRegisterAccessorImpl, UserType,
        boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >*, (void*)accessor );
    auto temp = boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >(ptr);
    delete ptr;
    return temp;
  }

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
