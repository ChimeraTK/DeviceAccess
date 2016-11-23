/*
 * Application.cc
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#include <string>
#include <thread>
#include <exception>

#include <boost/fusion/container/map.hpp>

#include <libxml++/libxml++.h>

#include <mtca4u/BackendFactory.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "Accessor.h"
#include "DeviceAccessor.h"
#include "FanOut.h"
#include "VariableNetworkNode.h"
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "ConstantAccessor.h"

using namespace ChimeraTK;

/*********************************************************************************************************************/

void Application::initialise() {

  // call the user-defined defineConnections() function which describes the structure of the application
  defineConnections();
  
  // connect any unconnected accessors with constant values
  processUnconnectedNodes();

  // realise the connections between variable accessors as described in the initialise() function
  makeConnections();
}

/*********************************************************************************************************************/

void Application::processUnconnectedNodes() {
  for(auto &module : overallModuleList) {
    for(auto &accessor : module->getAccessorList()) {
      if(!accessor->getNode().hasOwner()) {
        std::cerr << "*** Warning: Variable '" << accessor->getName() << "' is not connected. "
                     "Reading will always result in 0, writing will be ignored." << std::endl;
        networkList.emplace_back();
        networkList.back().addNode(*accessor);
        
        bool makeFeeder = !(networkList.back().hasFeedingNode());
        size_t length = accessor->getNumberOfElements();
        
        if(accessor->getNode().getValueType() == typeid(int8_t)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<int8_t>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(uint8_t)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<uint8_t>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(int16_t)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<int16_t>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(uint16_t)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<uint16_t>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(int32_t)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<int32_t>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(uint32_t)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<uint32_t>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(float)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<float>(makeFeeder, 0, length));
        }
        else if(accessor->getNode().getValueType() == typeid(double)) {
          constantList.emplace_back(VariableNetworkNode::makeConstant<double>(makeFeeder, 0, length));
        }
        else {
          throw std::invalid_argument("Unknown value type.");
        }

        networkList.back().addNode(constantList.back());
      }
    }
  }
}

/*********************************************************************************************************************/

void Application::checkConnections() {

  // check all networks for validity
  for(auto &network : networkList) {
    network.check();
  }
  
  // check if all accessors are connected
  for(auto &module : overallModuleList) {
    for(auto &accessor : module->getAccessorList()) {
      if(!accessor->getNode().hasOwner()) {
        throw std::invalid_argument("The accessor '"+accessor->getName()+"' of the module '"+module->getName()+
                                    "' was not connected!");
      }
    }
  }
  
}

/*********************************************************************************************************************/

void Application::run() {

  // check if the application name has been set
  if(applicationName == "") {
    throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
        "Error: An instance of Application must have its applicationName set.");
  }

  // start the necessary threads for the FanOuts etc.
  for(auto &adapter : adapterList) {
    adapter->activate();
  }

  // read all input variables once, to set the startup value e.g. coming from the config file
  // (without triggering an action inside the application)
  for(auto &module : overallModuleList) {
    for(auto &variable : module->getAccessorList()) {
      if(variable->getDirection() == VariableDirection::consuming) {
        variable->readNonBlocking();
      }
    }
  }

  // start the threads for the modules
  for(auto &module : overallModuleList) {
    module->run();
  }
}

/*********************************************************************************************************************/

void Application::shutdown() {

  // deactivate the FanOuts first, since they have running threads inside accessing the modules etc.
  // (note: the modules are members of the Application implementation and thus get destroyed after this destructor)
  for(auto &adapter : adapterList) {
    adapter->deactivate();
  }

  // next deactivate the modules, as they have running threads inside as well
  for(auto &module : overallModuleList) {
    module->terminate();
  }

  ApplicationBase::shutdown();

}
/*********************************************************************************************************************/

void Application::generateXML() {
  
  // define the connections
  defineConnections();
  
  // also search for unconnected nodes - this is here only executed to print the warnings
  processUnconnectedNodes();

  // check if the application name has been set
  if(applicationName == "") {
    throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
        "Error: An instance of Application must have its applicationName set.");
  }

  // create XML document with root node
  xmlpp::Document doc;
  xmlpp::Element *rootElement = doc.create_root_node("application", "https://github.com/ChimeraTK/ApplicationCore");
  rootElement->set_attribute("name",applicationName);

  for(auto &network : networkList) {

    // perform checks
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

VariableNetwork& Application::connect(VariableNetworkNode a, VariableNetworkNode b) {

  // if one of the nodes has the value type AnyType, set it to the type of the other
  // if both are AnyType, nothing changes.
  if(a.getValueType() == typeid(AnyType)) {
    a.setValueType(b.getValueType());
  }
  else if(b.getValueType() == typeid(AnyType)) {
    b.setValueType(a.getValueType());
  }

  // if one of the nodes has not yet a defined number of elements, set it to the number of elements of the other.
  // if both are undefined, nothing changes.
  if(a.getNumberOfElements() == 0) {
    a.setNumberOfElements(b.getNumberOfElements());
  }
  else if(b.getNumberOfElements() == 0) {
    b.setNumberOfElements(a.getNumberOfElements());
  }
  if(a.getNumberOfElements() != b.getNumberOfElements()) {
    throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
          "Error: Cannot connect array variables with difference number of elements!");
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

template<typename UserType>
boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> Application::createDeviceVariable(const std::string &deviceAlias,
    const std::string &registerName, VariableDirection direction, UpdateMode mode, size_t nElements) {

  // open device if needed
  if(deviceMap.count(deviceAlias) == 0) {
    deviceMap[deviceAlias] = mtca4u::BackendFactory::getInstance().createBackend(deviceAlias);
    deviceMap[deviceAlias]->open();
  }

  // use wait_for_new_data mode if push update mode was requested
  mtca4u::AccessModeFlags flags{};
  if(mode == UpdateMode::push && direction == VariableDirection::consuming) flags = {AccessMode::wait_for_new_data};

  // create DeviceAccessor for the proper UserType
  boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;
  auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<UserType>(registerName, nElements, 0, flags);
  impl.reset(new DeviceAccessor<UserType>(regacc, direction, mode));

  return impl;
}

/*********************************************************************************************************************/

template<typename UserType>
boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> Application::createProcessVariable(VariableNetworkNode const &node) {

  // determine the SynchronizationDirection
  SynchronizationDirection dir;
  if(node.getDirection() == VariableDirection::feeding) {
    dir = SynchronizationDirection::controlSystemToDevice;
  }
  else {
    dir = SynchronizationDirection::deviceToControlSystem;
  }

  // create the ProcessScalar for the proper UserType
  return _processVariableManager->createProcessArray<UserType>(dir, node.getPublicName(), node.getNumberOfElements(),
                                                               node.getOwner().getUnit(), node.getOwner().getDescription());
}

/*********************************************************************************************************************/

template<typename UserType>
std::pair< boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>, boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> >
  Application::createApplicationVariable(size_t nElements) {

  // create the ProcessScalar for the proper UserType
  return createSynchronizedProcessArray<UserType>(nElements);
}

/*********************************************************************************************************************/

void Application::makeConnections() {
  
  // run checks first
  checkConnections();

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

  // if the trigger type is external, create the trigger first
  if(network.getFeedingNode().hasExternalTrigger()) {
    VariableNetwork &dependency = network.getFeedingNode().getExternalTrigger().getOwner();
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
    boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingImpl;
    if(feeder.getType() == NodeType::Device) {
      feedingImpl = createDeviceVariable<UserType>(feeder.getDeviceAlias(), feeder.getRegisterName(),
          VariableDirection::consuming, feeder.getMode(), feeder.getNumberOfElements());
    }
    else if(feeder.getType() == NodeType::ControlSystem) {
      feedingImpl = createProcessVariable<UserType>(feeder);
    }
    else if(feeder.getType() == NodeType::Constant) {
      feedingImpl = boost::dynamic_pointer_cast<mtca4u::NDRegisterAccessor<UserType>>(feeder.getConstAccessor());
      assert(feedingImpl != nullptr);
    }
    else {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
    }

    // if we just have two nodes, directly connect them
    if(nNodes == 2 && !useExternalTrigger) {
      auto consumer = consumers.front();
      if(consumer.getType() == NodeType::Application) {
        consumer.getAppAccessor().useProcessVariable(feedingImpl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Device) {
        auto consumingImpl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
        // connect the Device with e.g. a ControlSystem node via an ImplementationAdapter
        adapterList.push_back(boost::shared_ptr<ImplementationAdapterBase>(
            new ImplementationAdapter<UserType>(consumingImpl,feedingImpl)));
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto consumingImpl = createProcessVariable<UserType>(consumer);
        // connect the ControlSystem with e.g. a Device node via an ImplementationAdapter
        adapterList.push_back(boost::shared_ptr<ImplementationAdapterBase>(
            new ImplementationAdapter<UserType>(consumingImpl,feedingImpl)));
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        consumer.getTriggerReceiver().getOwner().setExternalTriggerImpl(feedingImpl);
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
            auto impls = createApplicationVariable<UserType>(consumer.getNumberOfElements());
            fanOut->addSlave(impls.first);
            consumer.getAppAccessor().useProcessVariable(impls.second);
          }
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto impl = createProcessVariable<UserType>(consumer);
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::Device) {
          auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
              VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          auto impls = createApplicationVariable<UserType>(consumer.getNumberOfElements());
          fanOut->addSlave(impls.first);
          consumer.getTriggerReceiver().getOwner().setExternalTriggerImpl(impls.second);
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
        auto impls = createApplicationVariable<UserType>(consumer.getNumberOfElements());
        feeder.getAppAccessor().useProcessVariable(impls.first);
        consumer.getAppAccessor().useProcessVariable(impls.second);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto impl = createProcessVariable<UserType>(consumer);
        feeder.getAppAccessor().useProcessVariable(impl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Device) {
        auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
        feeder.getAppAccessor().useProcessVariable(impl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        auto impls = createApplicationVariable<UserType>(consumer.getNumberOfElements());
        feeder.getAppAccessor().useProcessVariable(impls.first);
        consumer.getTriggerReceiver().getOwner().setExternalTriggerImpl(impls.second);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Constant) {
        auto impl = consumer.getConstAccessor();
        feeder.getAppAccessor().useProcessVariable(impl);
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
          auto impls = createApplicationVariable<UserType>(consumer.getNumberOfElements());
          fanOut->addSlave(impls.first);
          consumer.getAppAccessor().useProcessVariable(impls.second);
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto impl = createProcessVariable<UserType>(consumer);
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::Device) {
          auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
              VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
          fanOut->addSlave(impl);
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          auto impls = createApplicationVariable<UserType>(consumer.getNumberOfElements());
          fanOut->addSlave(impls.first);
          consumer.getTriggerReceiver().getOwner().setExternalTriggerImpl(impls.second);
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

VariableNetwork& Application::createNetwork() {
  networkList.emplace_back();
  return networkList.back();
}

/*********************************************************************************************************************/

Application& Application::getInstance() {
  return dynamic_cast<Application&>(ApplicationBase::getInstance());
}
