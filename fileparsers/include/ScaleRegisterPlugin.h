/*
 * ScaleRegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_SCALE_REGISTER_PLUGIN_H
#define MTCA4U_SCALE_REGISTER_PLUGIN_H

#include "RegisterPlugin.h"
#include "BufferingRegisterAccessorImpl.h"

namespace mtca4u {

  /** RegisterPlugin to scale the register content with a given factor. */
  class ScaleRegisterPlugin : public RegisterPlugin {
  
    public:
      
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >* getBufferingRegisterAccessorImpl(void *accessor_ptr);

      VIRTUAL_FUNCTION_TEMPLATE_DECLARATION(getBufferingRegisterAccessorImpl, void*);

      static boost::shared_ptr<RegisterPlugin> createInstance(const std::map<std::string, Value<std::string> > &parameters);

    protected:

      /** constructor, only internally called from createInstance() */
      ScaleRegisterPlugin(const std::map<std::string, Value<std::string> > &parameters);

      /** The scaling factor to multiply the data with */
      Value<double> scalingFactor;
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
