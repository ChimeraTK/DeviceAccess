/*
 * Application.cc
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#include <string>
#include <thread>

#include <boost/fusion/container/map.hpp>

#include <libxml++/libxml++.h>

#include <mtca4u/BackendFactory.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "Accessor.h"
#include "DeviceAccessor.h"
#include "FanOut.h"
#include "VariableNetworkNode.h"

using namespace ChimeraTK;

Application *Application::instance = nullptr;
std::mutex Application::instance_mutex;

/*********************************************************************************************************************/

Application::Application(const std::string& name)
: applicationName(name)
{
  std::lock_guard<std::mutex> lock(instance_mutex);
  if(instance != nullptr) {
    throw std::string("Multiple instances of ChimeraTK::Application cannot be created."); // @todo TODO throw proper exception
  }
  instance = this;
}

/*********************************************************************************************************************/

Application::~Application() {
  std::lock_guard<std::mutex> lock(instance_mutex);
  instance = nullptr;
}

/*********************************************************************************************************************/

void Application::run() {
  // call the user-defined initialise() function which describes the structure of the application
  initialise();
  // realise the connections between variable accessors as described in the initialise() function
  makeConnections();
  // start the necessary threads for the FanOuts etc.
  for(auto &adapter : adapterList) {
    adapter->activate();
  }
  // start the threads for the modules
  for(auto module : moduleList) {
    module->run();
  }
}

/*********************************************************************************************************************/

void Application::generateXML() {
  initialise();

  // create XML document with root node
  xmlpp::Document doc;
  xmlpp::Element *rootElement = doc.create_root_node("application", "https://github.com/ChimeraTK/ApplicationCore");
  rootElement->set_attribute("name",applicationName);

  for(auto &network : networkList) {

    // perform checks
    network.dump();
    network.check();

    // create xml code for the feeder (if it is a control system node)
    auto feeder = network.getFeedingNode();
    feeder.createXML(rootElement);

    // create xml code for the consumers
    for(auto &consumer : network.getConsumingNodes()) {
      consumer.createXML(rootElement);
    }

  }
  /// @todo TODO don't write to stdout but to a file...
  doc.write_to_file_formatted(applicationName+".xml");
}

/*********************************************************************************************************************/

/*void Application::connectAccessors(AccessorBase &a, AccessorBase &b) {
  VariableNetwork &network = findOrCreateNetwork(&a,&b);
  network.addNode(a);
  network.addNode(b);
}*/

/*********************************************************************************************************************/

VariableNetwork& Application::connect(VariableNetworkNode a, VariableNetworkNode b) {

  // if one of the nodes as the value type AnyType, set it to the type of the other
  // if both are AnyType, nothing changes.
  if(a.getValueType() == typeid(AnyType)) {
    a.setValueType(b.getValueType());
  }
  else if(b.getValueType() == typeid(AnyType)) {
    b.setValueType(a.getValueType());
  }

  // if both nodes already have an owner, we are done
  if(a.hasOwner() && b.hasOwner()) {
    assert( &(a.getOwner()) == &(b.getOwner()) );   /// @todo TODO merge networks?
  }
  // add b to the existing network of a
  else if(a.hasOwner()) {
    a.getOwner().addNode(b);
  }

  // add a to the existing network of b
  else if(b.hasOwner()) {
    b.getOwner().addNode(a);
  }
  // create new network
  else {
    networkList.emplace_back();
    networkList.back().addNode(a);
    networkList.back().addNode(b);
  }
  return a.getOwner();
}

/*********************************************************************************************************************/

VariableNetworkNode Application::devReg(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode,
    const std::type_info &valTyp) {
  return{deviceAlias, registerName, mode, VariableDirection::invalid, valTyp};
}

/*********************************************************************************************************************/

template<typename UserType>
VariableNetworkNode Application::devReg(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode) {
  return devReg(deviceAlias, registerName, mode, typeid(UserType));
}

template VariableNetworkNode Application::devReg<int8_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<uint8_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<int16_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<uint16_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<int32_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<uint32_t>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<float>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
template VariableNetworkNode Application::devReg<double>(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);

/*********************************************************************************************************************/

template<typename UserType>
VariableNetworkNode Application::ctrlVar(const std::string &publicName) {
  return ctrlVar(publicName, typeid(UserType));
}

template VariableNetworkNode Application::ctrlVar<int8_t>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<uint8_t>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<int16_t>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<uint16_t>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<int32_t>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<uint32_t>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<float>(const std::string &publicName);
template VariableNetworkNode Application::ctrlVar<double>(const std::string &publicName);

/*********************************************************************************************************************/

VariableNetworkNode Application::ctrlVar(const std::string &publicName, const std::type_info &valTyp) {
  return {publicName, VariableDirection::invalid, valTyp};
}

/*********************************************************************************************************************/

VariableNetworkNode Application::feedingDevReg(const std::string &deviceAlias, const std::string &registerName,
    UpdateMode mode, const std::type_info &valTyp) {
  return {deviceAlias, registerName, mode, VariableDirection::feeding, valTyp};
}

/*********************************************************************************************************************/

template<typename UserType>
VariableNetworkNode Application::feedingDevReg(const std::string &deviceAlias, const std::string &registerName,
    UpdateMode mode) {
  return feedingDevReg(deviceAlias, registerName, mode, typeid(UserType));
}

/*********************************************************************************************************************/

VariableNetworkNode Application::consumingCtrlVar(const std::string &publicName, const std::type_info &valTyp) {
  return {publicName, VariableDirection::consuming, valTyp};
}

/*********************************************************************************************************************/

template<typename UserType>
VariableNetworkNode Application::consumingCtrlVar(const std::string &publicName) {
  return consumingCtrlVar(publicName, typeid(UserType));
}

/*********************************************************************************************************************/

VariableNetworkNode Application::feedingCtrlVar(const std::string &publicName, const std::type_info &valTyp) {
  return {publicName, VariableDirection::feeding, valTyp};
}

/*********************************************************************************************************************/

template<typename UserType>
VariableNetworkNode Application::feedingCtrlVar(const std::string &publicName) {
  return feedingCtrlVar(publicName, typeid(UserType));
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
  if(mode == UpdateMode::push && direction == VariableDirection::consuming) flags = {AccessMode::wait_for_new_data};

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
  // make the connections for all networks
  for(auto &network : networkList) {
    makeConnectionsForNetwork(network);
  }
}

/*********************************************************************************************************************/

void Application::dumpConnections() {
  std::cout << "==== List of all variable connections of the current Application ====" << std::endl;
  for(auto &network : networkList) {
    network.dump();
  }
  std::cout << "=====================================================================" << std::endl;
}

/*********************************************************************************************************************/

void Application::makeConnectionsForNetwork(VariableNetwork &network) {
  // if the network has been created already, do nothing
  if(network.isCreated()) return;

  // check if the network is legal
  network.check();

  // if the trigger type is external, create the trigger first
  if(network.getTriggerType() == VariableNetwork::TriggerType::external) {
    VariableNetwork &dependency = network.getExternalTrigger();
    if(!dependency.isCreated()) makeConnectionsForNetwork(dependency);
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

  // mark the network as created
  network.markCreated();
}

/*********************************************************************************************************************/

template<typename UserType>
void Application::typedMakeConnection(VariableNetwork &network) {
  bool connectionMade = false;  // to check the logic...

  size_t nNodes = network.countConsumingNodes()+1;
  auto feeder = network.getFeedingNode();
  auto consumers = network.getConsumingNodes();
  bool useExternalTrigger = network.getTriggerType() == VariableNetwork::TriggerType::external;
  bool useFeederTrigger = network.getTriggerType() == VariableNetwork::TriggerType::feeder;

  boost::shared_ptr<FanOut<UserType>> fanOut;

  // 1st case: the feeder requires a fixed implementation
  if(feeder.hasImplementation()) {

    // Create feeding implementation. Note: though the implementation is derived from the feeder, it will be used as
    // the implementation of the (or one of the) consumer. Logically, implementations are always pairs of
    // implementations (sender and receiver), but in this case the feeder already has a fixed implementation pair.
    // So our feedingImpl will contain the consumer-end of the implementation pair. This is the reason why the
    // functions createProcessScalar() and createDeviceAccessor() get the VariableDirection::consuming.
    boost::shared_ptr<mtca4u::ProcessVariable> feedingImpl;
    if(feeder.getType() == NodeType::Device) {
      feedingImpl = createDeviceAccessor<UserType>(feeder.getDeviceAlias(), feeder.getRegisterName(),
          VariableDirection::consuming, feeder.getMode());
    }
    else if(feeder.getType() == NodeType::ControlSystem) {
      feedingImpl = createProcessScalar<UserType>(VariableDirection::consuming, feeder.getPublicName());
    }

    // if we just have two nodes, directly connect them
    if(nNodes == 2 && !useExternalTrigger) {
      auto consumer = consumers.front();
      if(consumer.getType() == NodeType::Application) {
        consumer.getAppAccessor().useProcessVariable(feedingImpl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Device) {
        auto consumingImpl = createDeviceAccessor<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode());
        // connect the Device with e.g. a ControlSystem node via an ImplementationAdapter
        adapterList.push_back(boost::shared_ptr<ImplementationAdapterBase>(
            new ImplementationAdapter<UserType>(consumingImpl,feedingImpl)));
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto consumingImpl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.getPublicName());
        // connect the ControlSystem with e.g. a Device node via an ImplementationAdapter
        adapterList.push_back(boost::shared_ptr<ImplementationAdapterBase>(
            new ImplementationAdapter<UserType>(consumingImpl,feedingImpl)));
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        consumer.getTriggerReceiver().setExternalTriggerImpl(feedingImpl);
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else { /* !(nNodes == 2 && !useExternalTrigger) */
      // create FanOut
      fanOut.reset(new FanOut<UserType>(feedingImpl));

      // use FanOut as implementation for the first application consumer node, add all others as slaves
      // @todo TODO need a more sophisticated logic to take care of the UpdateMode
      bool isFirst = true;
      // if external trigger is enabled, add it to FanOut
      if(useExternalTrigger) {
        isFirst = false; // don't use the FanOut as an implementation if we have an external trigger
        fanOut->addExternalTrigger(network.getExternalTriggerImpl());
      }
      // if the trigger is provided by the pushing feeder, use the treaded version of the FanOut to distribute
      // new values immediately to all consumers.
      else if(useFeederTrigger) {
        isFirst = false;
      }
      for(auto &consumer : consumers) {
        if(consumer.getType() == NodeType::Application) {
          if(isFirst) {
            consumer.getAppAccessor().useProcessVariable(fanOut);
            isFirst = false;
          }
          else {
            auto impls = createProcessScalar<UserType>();
            fanOut->addSlave(impls.first);
            consumer.getAppAccessor().useProcessVariable(impls.second);
          }
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto impl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.getPublicName());
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::Device) {
          auto impl = createDeviceAccessor<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
              VariableDirection::feeding, consumer.getMode());
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          auto impls = createProcessScalar<UserType>();
          fanOut->addSlave(impls.first);
          consumer.getTriggerReceiver().setExternalTriggerImpl(impls.second);
        }
        else {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
        }
      }
      if(isFirst || useExternalTrigger || useFeederTrigger) { // FanOut wasn't used as implementation: store to list to keep it alive
        adapterList.push_back(fanOut);
      }
      connectionMade = true;
    }
  }
  // 2nd case: the feeder does not require a fixed implementation
  else {    /* !feeder.hasImplementation() */
    // we should be left with an application feeder node
    if(feeder.getType() != NodeType::Application) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
    }
    assert(!useExternalTrigger);
    // if we just have two nodes, directly connect them
    if(nNodes == 2) {
      auto consumer = consumers.front();
      if(consumer.getType() == NodeType::Application) {
        auto impls = createProcessScalar<UserType>();
        feeder.getAppAccessor().useProcessVariable(impls.first);
        consumer.getAppAccessor().useProcessVariable(impls.second);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto impl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.getPublicName());
        feeder.getAppAccessor().useProcessVariable(impl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Device) {
        auto impl = createDeviceAccessor<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode());
        feeder.getAppAccessor().useProcessVariable(impl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        auto impls = createProcessScalar<UserType>();
        feeder.getAppAccessor().useProcessVariable(impls.first);
        consumer.getTriggerReceiver().setExternalTriggerImpl(impls.second);
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else {
      // create FanOut and use it as the feeder implementation
      fanOut.reset(new FanOut<UserType>());
      feeder.getAppAccessor().useProcessVariable(fanOut);

      for(auto &consumer : consumers) {
        if(consumer.getType() == NodeType::Application) {
          auto impls = createProcessScalar<UserType>();
          fanOut->addSlave(impls.first);
          consumer.getAppAccessor().useProcessVariable(impls.second);
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto impl = createProcessScalar<UserType>(VariableDirection::feeding, consumer.getPublicName());
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::Device) {
          auto impl = createDeviceAccessor<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
              VariableDirection::feeding, consumer.getMode());
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          auto impls = createProcessScalar<UserType>();
          fanOut->addSlave(impls.first);
          consumer.getTriggerReceiver().setExternalTriggerImpl(impls.second);
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
  networkList.emplace_back();
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
  networkList.emplace_back();
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

VariableNetwork& Application::createNetwork() {
  networkList.emplace_back();
  return networkList.back();
}

/*********************************************************************************************************************/

template<typename UserType>
void Application::feedDeviceRegisterToControlSystem(const std::string &deviceAlias, const std::string &registerName,
    const std::string& publicName, AccessorBase &trigger) {

  UpdateMode mode = UpdateMode::push;
  if(dynamic_cast<InvalidAccessor*>(&trigger) == nullptr) mode = UpdateMode::poll;

  //VariableNetworkNode n1{deviceAlias,registerName, mode, VariableDirection::feeding, typeid(UserType)};
  auto &net = connect(feedingDevReg<UserType>(deviceAlias,registerName, mode),
                      consumingCtrlVar<UserType>(publicName));

  if(dynamic_cast<InvalidAccessor*>(&trigger) == nullptr) net.addTrigger(findNetwork(&trigger));
}

template void Application::feedDeviceRegisterToControlSystem<int8_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<uint8_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<int16_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<uint16_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<int32_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<uint32_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<float>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);
template void Application::feedDeviceRegisterToControlSystem<double>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName, AccessorBase &trigger);

/*********************************************************************************************************************/

template<typename UserType>
void Application::consumeDeviceRegisterFromControlSystem(const std::string &deviceAlias, const std::string &registerName,
    const std::string& publicName) {
  networkList.emplace_back();
  VariableNetwork& network = networkList.back();
  network.addFeedingPublication(typeid(UserType), "arbitrary", publicName);
  network.addConsumingDeviceRegister(deviceAlias, registerName);
}

template void Application::consumeDeviceRegisterFromControlSystem<int8_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<uint8_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<int16_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<uint16_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<int32_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<uint32_t>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<float>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);
template void Application::consumeDeviceRegisterFromControlSystem<double>(const std::string &deviceAlias, const std::string &registerName, const std::string& publicName);

/*********************************************************************************************************************/
