/*
 * RegisterPlugin.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_REGISTER_INFO_PLUGIN_H
#define CHIMERA_TK_REGISTER_INFO_PLUGIN_H

#include <boost/smart_ptr/shared_ptr.hpp>

#include "ForwardDeclarations.h"

namespace ChimeraTK {

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
          const std::map<std::string, std::string > &parameters);

  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_REGISTER_INFO_PLUGIN_H */
