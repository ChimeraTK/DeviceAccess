/*
 * Application.cc
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#include <string>
#include <thread>

#include <boost/fusion/container/map.hpp>

#include "Application.h"
#include "ApplicationModule.h"
#include "PublishedAccessor.h"
#include "Accessor.h"

using namespace ChimeraTK;

Application *Application::instance = nullptr;
std::mutex Application::instance_mutex;

/*********************************************************************************************************************/

Application::Application() {
  std::lock_guard<std::mutex> lock(instance_mutex);
  if(instance != nullptr) {
    throw std::string("Multiple instances of ChimeraTK::Application cannot be created."); // @todo TODO throw proper exception
  }
  instance = this;
}

/*********************************************************************************************************************/

void Application::run() {
  initialise();
  makeConnections();
  for(auto module : moduleList) {
    module->run();
  }
}

/*********************************************************************************************************************/

void Application::connectAccessors(AccessorBase &a, AccessorBase &b) {
  if(a.isOutput() && !b.isOutput()) {
    connectionMap[&a].push_back(&b);
  }
  else if(b.isOutput() && !a.isOutput()) {
    connectionMap[&b].push_back(&a);
  }
  else {
    throw std::string("Cannot connect accessors with equal direction."); // @todo TODO throw proper exception
  }
  // @todo TODO check if the input accessor is already in the list of another output accessor
}

/*********************************************************************************************************************/

template<typename UserType>
void Application::publishAccessor(Accessor<UserType> &a, const std::string& name) {
  // @todo TODO check if already published and throw an error

  // turn around the direction: the PublishedAccessor represents the control-system-side part!
  VariableDirection direction;
  if(a.getDirection() == VariableDirection::input) {
    direction = VariableDirection::output;
  }
  else {
    direction = VariableDirection::input;
  }

  // first create a PublishedAccessor to represent the control-system-side part, and store it to the list
  boost::shared_ptr<AccessorBase> b;
  b.reset( new PublishedAccessor<UserType>(name, direction, a.getUnit()) );
  publicationList.push_back(b);

  // now connect the accessor with the PublishedAccessor
  connectAccessors(a, *b);
}

namespace {

  // TODO @todo this is copy&paste from SupportedUserTypes.h of DeviceAccess, but with string and 64-bit ints removed
  template< template<typename> class TemplateClass >
  class TemplateUserTypeMap {
    public:
      boost::fusion::map<
          boost::fusion::pair<int8_t, TemplateClass<int8_t> >,
          boost::fusion::pair<uint8_t, TemplateClass<uint8_t> >,
          boost::fusion::pair<int16_t, TemplateClass<int16_t> >,
          boost::fusion::pair<uint16_t, TemplateClass<uint16_t> >,
          boost::fusion::pair<int32_t, TemplateClass<int32_t> >,
          boost::fusion::pair<uint32_t, TemplateClass<uint32_t> >,
          boost::fusion::pair<float, TemplateClass<float> >,
          boost::fusion::pair<double, TemplateClass<double> >
       > table;
  };


  template<typename UserType>
  class Application_publishAccessor_instantiator {
    public:
      Application_publishAccessor_instantiator() : pointerToMember(&Application::publishAccessor<UserType>) {
      }
      void (Application::*pointerToMember)(Accessor<UserType>&, const std::string&);
  };
  TemplateUserTypeMap<Application_publishAccessor_instantiator> x;
}

/*********************************************************************************************************************/

void Application::makeConnections() {

  // make connections between accessors
  for(auto conn : connectionMap) {
    // make connection in single target case
    if(conn.second.size() == 1 ) {
      if(!conn.second.front()->isInitialised()) {
        auto pv = conn.first->createProcessVariable();
        conn.second.front()->useProcessVariable(pv);
      }
      else {
        auto pv = conn.second.front()->createProcessVariable();
        conn.first->useProcessVariable(pv);
      }
    }
    else {
      throw std::string("Connecting to multiple inputs is not yet implemented."); // @todo TODO implement
    }
  }
}

/*********************************************************************************************************************/
