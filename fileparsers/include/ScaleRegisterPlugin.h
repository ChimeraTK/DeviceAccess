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

      static boost::shared_ptr<RegisterPlugin> createInstance(const std::map<std::string, Value<std::string> > &parameters);

      virtual boost::shared_ptr<RegisterAccessor> decorateRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor);

    protected:

      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > decorateBufferingRegisterAccessor_impl(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const;
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(ScaleRegisterPlugin, decorateBufferingRegisterAccessor_impl, 1);

      /** constructor, only internally called from createInstance() */
      ScaleRegisterPlugin(const std::map<std::string, Value<std::string> > &parameters);

      /** The scaling factor to multiply the data with */
      Value<double> scalingFactor;
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_H */
