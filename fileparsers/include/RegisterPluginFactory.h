/*
 * RegisterPluginFactory.h
 *
 *  Created on: Feb 25, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PLUGIN_FACTORY_H
#define MTCA4U_REGISTER_PLUGIN_FACTORY_H

#include <map>

#include "RegisterInfoPlugin.h"
#include "DynamicValue.h"

namespace ChimeraTK {

  /** Factory for register plugins. Plugins need to register with the factory. */
  class RegisterPluginFactory {
  
    public:

      /** Static function to get an instance of the factory */
      static RegisterPluginFactory& getInstance();

      /** Function to create a plugin instance */
      boost::shared_ptr<RegisterInfoPlugin> createPlugin(const std::string &name,
          const std::map<std::string, DynamicValue<std::string> > &parameters);

      /** Function to register a plugin */
      void registerPlugin(std::string name,
          boost::shared_ptr<RegisterInfoPlugin> (*creatorFunction)(const std::map<std::string, DynamicValue<std::string> > &parameters));

    private:

      /** Private constructor to avoid instantiation of this singleton */    
      RegisterPluginFactory();

      /** To avoid making copies */
      RegisterPluginFactory(RegisterPluginFactory const&);
      RegisterPluginFactory(RegisterPluginFactory const&&);
      void operator=(RegisterPluginFactory const&);
      
      /** Map holding pointers to the creator functions for each plugin */
      std::map<std::string,
          boost::shared_ptr<RegisterInfoPlugin> (*)(const std::map<std::string, DynamicValue<std::string> > &parameters)> creatorMap;
  
  };

} /* namespace ChimeraTK */

#endif /* MTCA4U_REGISTER_PLUGIN_FACTORY_H */
