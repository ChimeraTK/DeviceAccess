/*
 * Application.cc
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#include <string>
#include <thread>

#include <boost/fusion/container/map.hpp>

#include <mtca4u/BackendFactory.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "PublishedAccessor.h"
#include "Accessor.h"
#include "DeviceAccessor.h"

using namespace ChimeraTK;

Application *Application::instance = nullptr;
std::mutex Application::instance_mutex;


namespace {
  // TODO @todo this is copy&paste from SupportedUserTypes.h of DeviceAccess, but with string and 64-bit ints removed
  template< template<typename> class TemplateClass >
  class myTemplateUserTypeMap {
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
}

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

template void Application::publishAccessor(Accessor<int8_t> &a, const std::string& name);
template void Application::publishAccessor(Accessor<uint8_t> &a, const std::string& name);
template void Application::publishAccessor(Accessor<int16_t> &a, const std::string& name);
template void Application::publishAccessor(Accessor<uint16_t> &a, const std::string& name);
template void Application::publishAccessor(Accessor<int32_t> &a, const std::string& name);
template void Application::publishAccessor(Accessor<uint32_t> &a, const std::string& name);
template void Application::publishAccessor(Accessor<float> &a, const std::string& name);
template void Application::publishAccessor(Accessor<double> &a, const std::string& name);

/*********************************************************************************************************************/

template<typename UserType>
void Application::connectAccessorToDevice(Accessor<UserType> &a, const std::string &deviceAlias,
    const std::string &registerName, UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {

  // open device if needed
  if(deviceMap.count(deviceAlias) == 0) {
    deviceMap[deviceAlias] = mtca4u::BackendFactory::getInstance().createBackend(deviceAlias);
    deviceMap[deviceAlias]->open();
  }

  // obtain accessor from device
  mtca4u::AccessModeFlags flags{};
  if(mode == UpdateMode::push) flags = {AccessMode::wait_for_new_data};
  auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<UserType>(registerName,
      numberOfElements, elementOffsetInRegister, flags);

  // "convert" DeviceAccess accessor into a ProcessScalar implementation
  boost::shared_ptr<mtca4u::ProcessVariable> impl( new DeviceAccessor<UserType>(regacc,a.getDirection(),mode) );

  // the actual connection is done later in makeConnections(), so just save the accessor and the implementation to a map
  deviceAccessorMap[&a] = impl;
}

template void Application::connectAccessorToDevice(Accessor<int8_t> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<uint8_t> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<int16_t> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<uint16_t> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<int32_t> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<uint32_t> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<float> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);
template void Application::connectAccessorToDevice(Accessor<double> &a, const std::string&, const std::string &, UpdateMode, size_t, size_t);

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

  // make connections to devices
  for(auto devconn : deviceAccessorMap) {
    if(devconn.first->isInitialised()) {
      throw std::string("Connecting to multiple inputs is not yet implemented (2)."); // @todo TODO implement
    }
    devconn.first->useProcessVariable(devconn.second);
  }

}

/*********************************************************************************************************************/
