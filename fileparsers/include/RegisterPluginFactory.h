/*
 * RegisterPluginFactory.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PLUGIN_FACTORY_H
#define MTCA4U_REGISTER_PLUGIN_FACTORY_H

#include <map>

#include "Value.h"
#include "RegisterPlugin.h"

namespace mtca4u {

  /** Factory for register plugins. Plugins need to register with the factory. */
  class RegisterPluginFactory {
  
    public:

      /** Static function to get an instance of the factory */
      static RegisterPluginFactory& getInstance();

      /** Function to create a plugin instance */
      boost::shared_ptr<RegisterPlugin> createPlugin(const std::string &name,
          const std::map<std::string, DynamicValue<std::string> > &parameters);

      /** Function to register a plugin */
      void registerPlugin(std::string name,
          boost::shared_ptr<RegisterPlugin> (*creatorFunction)(const std::map<std::string, DynamicValue<std::string> > &parameters));

    private:

      /** Private constructor to avoid instantiation of this singleton */    
      RegisterPluginFactory();

      /** To avoid making copies */
      RegisterPluginFactory(RegisterPluginFactory const&);
      RegisterPluginFactory(RegisterPluginFactory const&&);
      void operator=(RegisterPluginFactory const&);
      
      /** Map holding pointers to the creator functions for each plugin */
      std::map<std::string,
          boost::shared_ptr<RegisterPlugin> (*)(const std::map<std::string, DynamicValue<std::string> > &parameters)> creatorMap;
  
  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PLUGIN_FACTORY_H */
