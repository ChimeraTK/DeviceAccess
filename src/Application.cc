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
  if(direction == VariableDirection::consuming) {
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

  size_t nNodes = network.countConsumingNodes()+1;
  auto &feeder = network.getFeedingNode();
  auto &consumers = network.getConsumingNodes();

  boost::shared_ptr<FanOut<UserType>> fanOut;

  // 1st case: the feeder requires a fixed implementation
  if(feeder.hasImplementation()) {

    // Create feeding implementation. Note: though the implementation is derived from the feeder, it will be used as
    // the implementation of the (or one of the) consumer. Logically, implementations are always pairs of
    // implementations (sender and receiver), but in this case the feeder already has a fixed implementation pair.
    // So our feedingImpl will contain the consumer-end of the implementation pair. This is the reason why the
    // functions createProcessScalar() and createDeviceAccessor() get the VariableDirection::consuming.
    boost::shared_ptr<mtca4u::ProcessVariable> feedingImpl;
    if(feeder.type == VariableNetwork::NodeType::Device) {
      feedingImpl = createDeviceAccessor<UserType>(feeder.deviceAlias, feeder.registerName,
          VariableDirection::consuming, feeder.mode);
    }
    else if(feeder.type == VariableNetwork::NodeType::ControlSystem) {
      feedingImpl = createProcessScalar<UserType>(VariableDirection::consuming, feeder.publicName);
    }

    // if we just have two nodes, directly connect them
    if(nNodes == 2) {
      auto consumer = consumers.front();
      if(consumer.type == VariableNetwork::NodeType::Application) {
        consumer.appNode->useProcessVariable(feedingImpl);
        connectionMade = true;
      }
      else if(consumers.front().type == VariableNetwork::NodeType::Device) {
        auto consumingImpl = createDeviceAccessor<UserType>(consumer.deviceAlias, consumer.registerName,
            VariableDirection::feeding, consumer.mode);
        // connect the Device with e.g. a ControlSystem node via an ImplementationAdapter
        adapterList.push_back(boost::shared_ptr<ImplementationAdapterBase>(
            new ImplementationAdapter<UserType>(consumingImpl,feedingImpl)));
        connectionMade = true;
      }
      else if(consumer.type == VariableNetwork::NodeType::ControlSystem) {
        auto consumingImpl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.publicName);
        // connect the Device with e.g. a ControlSystem node via an ImplementationAdapter
        adapterList.push_back(boost::shared_ptr<ImplementationAdapterBase>(
            new ImplementationAdapter<UserType>(consumingImpl,feedingImpl)));
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else {
      // create FanOut
      fanOut.reset(new FanOut<UserType>(feedingImpl));

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
          auto impl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.publicName);
          fanOut->addSlave(impl);
        }
        else if(consumer.type == VariableNetwork::NodeType::Device) {
          auto impl = createDeviceAccessor<UserType>(consumer.deviceAlias, consumer.registerName,
              VariableDirection::feeding, consumer.mode);
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
        auto impl = createProcessScalar<UserType>(VariableDirection::feeding, consumers.front().publicName);
        feeder.appNode->useProcessVariable(impl);
        connectionMade = true;
      }
      else if(consumers.front().type == VariableNetwork::NodeType::Device) {
        auto impl = createDeviceAccessor<UserType>(consumers.front().deviceAlias, consumers.front().registerName,
            VariableDirection::feeding, consumers.front().mode);
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
          auto impl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.publicName);
          fanOut->addSlave(impl);
        }
        else if(consumer.type == VariableNetwork::NodeType::Device) {
          auto impl = createDeviceAccessor<UserType>(consumer.deviceAlias, consumer.registerName,
              VariableDirection::feeding, consumer.mode);
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
        "The variable network cannot be handled. Implementation missing!");
  }

}

/*********************************************************************************************************************/

VariableNetwork& Application::findOrCreateNetwork(AccessorBase *a, AccessorBase *b) {

  // search for a and b in the networkList
  auto &na = findNetwork(a);
  auto &nb = findNetwork(b);

  // if both accessors are found, check if both are in the same network
  if(na != invalidNetwork && nb != invalidNetwork) {
    if(na == nb) {
      return na;
    }
    else {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
          "Trying to connect two accessors which already are part of a network.");
    }
  }
  // if only one accessor is found, return its network
  else if(na != invalidNetwork && nb == invalidNetwork) {
    return na;
  }
  else if(na == invalidNetwork && nb != invalidNetwork) {
    return nb;
  }
  // if no accessor is found, create a new network and add it to the list
  networkList.push_back({});
  return networkList.back();
}

/*********************************************************************************************************************/

VariableNetwork& Application::findOrCreateNetwork(AccessorBase *a) {

  // search in the networkList
  auto &na = findNetwork(a);

  // if the accessors is found, return its network
  if(na != invalidNetwork) {
    return na;
  }
  // if no accessor is found, create a new network and add it to the list
  networkList.push_back({});
  return networkList.back();
}

/*********************************************************************************************************************/

VariableNetwork& Application::findNetwork(AccessorBase *a) {

  // search for a and b in the networkList
  auto r = find_if(networkList.begin(), networkList.end(),
      [a](const VariableNetwork& n) { return n.hasAppNode(a); } );

  // if no network found, create one
  if(r == networkList.end()) {
    return invalidNetwork;
  }

  // return the found network
  return *r;
}

/*********************************************************************************************************************/

template<typename UserType>
void Application::feedDeviceRegisterToControlSystem(const std::string &deviceAlias, const std::string &registerName,
    UpdateMode mode, const std::string& publicName) {
  VariableNetwork network;
  network.addFeedingDeviceRegister(typeid(UserType), deviceAlias, registerName, mode);
  network.addConsumingPublication(publicName);
  networkList.push_back(network);
}

template void Application::feedDeviceRegisterToControlSystem<int8_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<uint8_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<int16_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<uint16_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<int32_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<uint32_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<float>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::feedDeviceRegisterToControlSystem<double>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);

/*********************************************************************************************************************/

template<typename UserType>
void Application::consumeDeviceRegisterFromControlSystem(const std::string &deviceAlias, const std::string &registerName,
    UpdateMode mode, const std::string& publicName) {
  VariableNetwork network;
  network.addFeedingPublication(typeid(UserType), publicName);
  network.addConsumingDeviceRegister(deviceAlias, registerName, mode);
  networkList.push_back(network);
}

template void Application::consumeDeviceRegisterFromControlSystem<int8_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<uint8_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<int16_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<uint16_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<int32_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<uint32_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<float>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<double>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, const std::string& publicName);

/*********************************************************************************************************************/
