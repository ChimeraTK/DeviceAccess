/*
 * RegisterPluginFactory.cc
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPluginFactory.h"

namespace mtca4u {

  RegisterPluginFactory::RegisterPluginFactory() {
  }

  /********************************************************************************************************************/

  RegisterPluginFactory& RegisterPluginFactory::getInstance() {
    static RegisterPluginFactory factoryInstance; /** Thread safe in C++11*/
    return factoryInstance;
  }

  /********************************************************************************************************************/

  void RegisterPluginFactory::registerPlugin(std::string name,
      boost::shared_ptr<RegisterPlugin> (*creatorFunction)(const std::map<std::string, DynamicValue<std::string> > &parameters)) {
    creatorMap[name] = creatorFunction;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterPlugin> RegisterPluginFactory::createPlugin(const std::string &name,
      const std::map<std::string, DynamicValue<std::string> > &parameters) {
    try {
      return creatorMap.at(name)(parameters);
    }
    catch(std::out_of_range &e) {
      throw DeviceException("RegisterPluginFactory::createPlugin(): unknown RegisterPlugin '"+name+"'.",
          DeviceException::WRONG_PARAMETER);
    }
  }

} /* namespace mtca4u */
