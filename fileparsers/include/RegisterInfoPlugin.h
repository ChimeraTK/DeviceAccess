/*
 * RegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_INFO_PLUGIN_H
#define MTCA4U_REGISTER_INFO_PLUGIN_H

#include <boost/smart_ptr/shared_ptr.hpp>

#include "ForwardDeclarations.h"

namespace mtca4u {

  /** Base class for plugins providing additional information in a RegisterInfo, e.g. coming from the map file. This
   *  base class is essentially empty, the derived plugin classes should contain public data members and implement the
   *  createInstance() function which is passed to RegisterPluginFactory::registerPlugin(). The data members should be
   *  filled by the createInstance() implementation. */
  class RegisterInfoPlugin {
  
    public:

      /** Virtual destructor needed to make the class polymorphic */
      virtual ~RegisterInfoPlugin() {}

      /** Derived classes need to implement this function to fill the data members from the parameter map. The pointer
       *  to this function needs to be passed to RegisterPluginFactory::registerPlugin(). */
      static boost::shared_ptr<RegisterInfoPlugin> createInstance(
          const std::map<std::string, DynamicValue<std::string> > &parameters);
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_INFO_PLUGIN_H */
