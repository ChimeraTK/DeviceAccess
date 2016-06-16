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
#include "Accessor.h"
#include "DeviceAccessor.h"
#include "FanOut.h"

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

void Application::generateXML() {
  initialise();

}

/*********************************************************************************************************************/

void Application::connectAccessors(AccessorBase &a, AccessorBase &b) {
  VariableNetwork &network = findOrCreateNetwork(&a,&b);
  network.addAppNode(a);
  network.addAppNode(b);
}

/*********************************************************************************************************************/

void Application::publishAccessor(AccessorBase &a, const std::string& name) {
  VariableNetwork &network = findOrCreateNetwork(&a);
  network.addAppNode(a);
  if(a.isFeeding()) {
    network.addConsumingPublication(name);
  }
  else {
    network.addFeedingPublication(a,name);
  }
}

/*********************************************************************************************************************/

void Application::connectAccessorToDevice(AccessorBase &a, const std::string &deviceAlias,
    const std::string &registerName, UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
  VariableNetwork &network = findOrCreateNetwork(&a);
  network.addAppNode(a);
  if(a.isFeeding()) {
    network.addConsumingDeviceRegister(deviceAlias, registerName, mode, numberOfElements, elementOffsetInRegister);
  }
  else {
    network.addFeedingDeviceRegister(a, deviceAlias, registerName, mode, numberOfElements, elementOffsetInRegister);
  }
}

/*********************************************************************************************************************/

template<typename UserType>
boost::shared_ptr<mtca4u::ProcessVariable> Application::createDeviceAccessor(const std::string &deviceAlias,
    const std::string &registerName, VariableDirection direction, UpdateMode mode) {

  // open device if needed
  if(deviceMap.count(deviceAlias) == 0) {
    deviceMap[deviceAlias] = mtca4u::BackendFactory::getInstance().createBackend(deviceAlias);
    deviceMap[deviceAlias]->open();
  }

  // use wait_for_new_data mode if push update mode was requested
  mtca4u::AccessModeFlags flags{};
  if(mode == UpdateMode::push) flags = {AccessMode::wait_for_new_data};

  // create DeviceAccessor for the proper UserType
  boost::shared_ptr<mtca4u::ProcessVariable> impl;
  auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<UserType>(registerName, 1, 0, flags);
  impl.reset(new DeviceAccessor<UserType>(regacc, direction, mode));

  return impl;
}

/*********************************************************************************************************************/

template<typename UserType>
boost::shared_ptr<mtca4u::ProcessVariable> Application::createProcessScalar(VariableDirection direction,
    const std::string &name) {

  // determine the SynchronizationDirection
  SynchronizationDirection dir;
  if(direction == VariableDirection::feeding) {
    dir = SynchronizationDirection::controlSystemToDevice;
  }
  else {
    dir = SynchronizationDirection::deviceToControlSystem;
  }

  // create the ProcessScalar for the proper UserType
  boost::shared_ptr<mtca4u::ProcessVariable> impl;
  impl = _processVariableManager->createProcessScalar<UserType>(dir, name);
  return impl;
}

/*********************************************************************************************************************/

template<typename UserType>
std::pair< boost::shared_ptr<mtca4u::ProcessVariable>, boost::shared_ptr<mtca4u::ProcessVariable> >
  Application::createProcessScalar() {

  // create the ProcessScalar for the proper UserType
  return createSynchronizedProcessScalar<UserType>();
}

/*********************************************************************************************************************/

void Application::makeConnections() {

  // make connections between accessors
  for(auto network : networkList) {

    // output network information
    network.dump();

    // check if network is legal
    if(network.countConsumingNodes() == 0) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Illegal variable network found: no consuming nodes connected!");
    }
    if(!network.hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Illegal variable network found: no feeding node connected!");
    }

    // defer actual network creation to templated function
    // @todo TODO replace with boost::mpl::for_each loop!
    if(network.getValueType() == typeid(int8_t)) {
      typedMakeConnection<int8_t>(network);
    }
    else if(network.getValueType() == typeid(uint8_t)) {
      typedMakeConnection<uint8_t>(network);
    }
    else if(network.getValueType() == typeid(int16_t)) {
      typedMakeConnection<int16_t>(network);
    }
    else if(network.getValueType() == typeid(uint16_t)) {
      typedMakeConnection<uint16_t>(network);
    }
    else if(network.getValueType() == typeid(int32_t)) {
      typedMakeConnection<int32_t>(network);
    }
    else if(network.getValueType() == typeid(uint32_t)) {
      typedMakeConnection<uint32_t>(network);
    }
    else if(network.getValueType() == typeid(float)) {
      typedMakeConnection<float>(network);
    }
    else if(network.getValueType() == typeid(double)) {
      typedMakeConnection<double>(network);
    }
  }

}

/*********************************************************************************************************************/

template<typename UserType>
void Application::typedMakeConnection(VariableNetwork &network) {
  bool connectionMade = false;  // to check the logic...

  // ??
  size_t nNodes = network.countConsumingNodes()+1;
  size_t nImpls = network.countFixedImplementations();
  if(nImpls > nNodes-1) {
    throw ApplicationExceptionWithID<ApplicationExceptionID::notYetImplemented>(
        "The variable network cannot be handled. Too many fixed implementations!"); // @todo TODO implement
  }

  auto &feeder = network.getFeedingNode();
  auto &consumers = network.getConsumingNodes();
  boost::shared_ptr<FanOut<UserType>> fanOut;

  // 1st case: the feeder requires a fixed implementation
  if(feeder.hasImplementation()) {

    // create feeder's implementation
    boost::shared_ptr<mtca4u::ProcessVariable> feederImpl;
    if(feeder.type == VariableNetwork::NodeType::Device) {
      feederImpl = createDeviceAccessor<UserType>(feeder.deviceAlias, feeder.registerName, VariableDirection::feeding,
          feeder.mode);
    }
    else if(feeder.type == VariableNetwork::NodeType::ControlSystem) {
      feederImpl = createProcessScalar<UserType>(VariableDirection::feeding, feeder.publicName);
    }

    // if we just have two nodes, directly connect them
    if(nNodes == 2) {
      if(consumers.front().type == VariableNetwork::NodeType::Application) {
        consumers.front().appNode->useProcessVariable(feederImpl);
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else {
      // create FanOut
      fanOut.reset(new FanOut<UserType>(feederImpl));

      // use FanOut as implementation for the first application consumer node, add all others as slaves
      // @todo TODO need a more sophisticated logic to take care of the UpdateMode
      bool isFirst = true;
      for(auto &consumer : consumers) {
        if(consumer.type == VariableNetwork::NodeType::Application) {
          if(isFirst) {
            consumer.appNode->useProcessVariable(fanOut);
            isFirst = false;
          }
          else {
            auto impls = createProcessScalar<UserType>();
            fanOut->addSlave(impls.first);
            consumer.appNode->useProcessVariable(impls.second);
          }
        }
        else if(consumer.type == VariableNetwork::NodeType::ControlSystem) {
          auto impl = createProcessScalar<UserType>(VariableDirection::consuming, consumer.publicName);
          fanOut->addSlave(impl);
        }
        else if(consumer.type == VariableNetwork::NodeType::Device) {
          auto impl = createDeviceAccessor<UserType>(consumer.deviceAlias, consumer.registerName,
              VariableDirection::consuming, consumer.mode);
          fanOut->addSlave(impl);
        }
        else {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
        }
      }
      if(isFirst) { // there was no application consumer node
        throw ApplicationExceptionWithID<ApplicationExceptionID::notYetImplemented>(
            "The variable network cannot be handled. Implementation missing!");
      }
      connectionMade = true;
    }
  }
  // 2nd case: the feeder does not require a fixed implementation
  else {
    // we should be left with an application feeder node
    if(feeder.type != VariableNetwork::NodeType::Application) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
    }
    // if we just have two nodes, directly connect them
    if(nNodes == 2) {
      if(consumers.front().type == VariableNetwork::NodeType::Application) {
        auto impls = createProcessScalar<UserType>();
        feeder.appNode->useProcessVariable(impls.first);
        consumers.front().appNode->useProcessVariable(impls.second);
        connectionMade = true;
      }
      else if(consumers.front().type == VariableNetwork::NodeType::ControlSystem) {
        auto impl = createProcessScalar<UserType>(VariableDirection::consuming, consumers.front().publicName);
        feeder.appNode->useProcessVariable(impl);
        connectionMade = true;
      }
      else if(consumers.front().type == VariableNetwork::NodeType::Device) {
        auto impl = createDeviceAccessor<UserType>(consumers.front().deviceAlias, consumers.front().registerName,
            VariableDirection::consuming, consumers.front().mode);
        feeder.appNode->useProcessVariable(impl);
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else {
      // create FanOut and use it as the feeder implementation
      fanOut.reset(new FanOut<UserType>());
      feeder.appNode->useProcessVariable(fanOut);

      for(auto &consumer : consumers) {
        if(consumer.type == VariableNetwork::NodeType::Application) {
          auto impls = createProcessScalar<UserType>();
          fanOut->addSlave(impls.first);
          consumer.appNode->useProcessVariable(impls.second);
        }
        else if(consumer.type == VariableNetwork::NodeType::ControlSystem) {
          auto impl = createProcessScalar<UserType>(VariableDirection::consuming, consumer.publicName);
          fanOut->addSlave(impl);
        }
        else if(consumer.type == VariableNetwork::NodeType::Device) {
          auto impl = createDeviceAccessor<UserType>(consumer.deviceAlias, consumer.registerName, VariableDirection::consuming,
              consumer.mode);
          fanOut->addSlave(impl);
        }
        else {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
        }
      }
      connectionMade = true;
    }
  }

  if(!connectionMade) {
    throw ApplicationExceptionWithID<ApplicationExceptionID::notYetImplemented>(
        "The variable network cannot be handled. Implementation missing!"); // @todo TODO implement
  }

}

/*********************************************************************************************************************/

VariableNetwork& Application::findOrCreateNetwork(AccessorBase *a, AccessorBase *b) {

  // search for a and b in the inputAccessorList
  auto r = find_if(networkList.begin(), networkList.end(),
      [a,b](const VariableNetwork& n) { return n.hasAppNode(a,b); } );

  // if no network found, create one
  if(r == networkList.end()) {
    networkList.push_back({});
    return networkList.back();
  }

  // store the reference to the found network
  VariableNetwork &ret = *r;

  // check if more than one network is found
  if(++r != networkList.end()) {
    throw std::string("Trying to connect two accessors which already are part of a network."); // @todo TODO throw proper exception
  }

  // return the found network
  return ret;
}

/*********************************************************************************************************************/
