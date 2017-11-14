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
#include "ThreadedFanOut.h"
#include "ConsumingFanOut.h"
#include "FeedingFanOut.h"
#include "TriggerFanOut.h"
#include "VariableNetworkNode.h"
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "ConstantAccessor.h"
#include "TestDecoratorRegisterAccessor.h"

using namespace ChimeraTK;

std::mutex Application::testableMode_mutex;

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

/** Functor class to create a constant for otherwise unconnected variables, suitable for boost::fusion::for_each(). */
namespace {
  struct CreateConstantForUnconnectedVar {
    CreateConstantForUnconnectedVar(const std::type_info &typeInfo, bool makeFeeder, size_t length)
    : _typeInfo(typeInfo), _makeFeeder(makeFeeder), _length(length) {}
    
    template<typename PAIR>
    void operator()(PAIR&) const {
      if(typeid(typename PAIR::first_type) != _typeInfo) return;
      theNode = VariableNetworkNode::makeConstant<typename PAIR::first_type>(_makeFeeder, 0, _length);
      done = true;
    }
    
    const std::type_info &_typeInfo;
    bool _makeFeeder;
    size_t _length;
    mutable bool done{false};
    mutable VariableNetworkNode theNode;
  };
}

/*********************************************************************************************************************/

void Application::processUnconnectedNodes() {
  for(auto &module : getSubmoduleListRecursive()) {
    for(auto &accessor : module->getAccessorList()) {
      if(!accessor.hasOwner()) {
        if(enableUnconnectedVariablesWarning) {
          std::cerr << "*** Warning: Variable '" << accessor.getName() << "' is not connected. "    // LCOV_EXCL_LINE
                       "Reading will always result in 0, writing will be ignored." << std::endl;    // LCOV_EXCL_LINE
        }
        networkList.emplace_back();
        networkList.back().addNode(accessor);
        
        bool makeFeeder = !(networkList.back().hasFeedingNode());
        size_t length = accessor.getNumberOfElements();
        auto callable = CreateConstantForUnconnectedVar(accessor.getValueType(), makeFeeder, length);
        boost::fusion::for_each(mtca4u::userTypeMap(), callable);
        assert(callable.done);
        constantList.emplace_back(callable.theNode);
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
  // note: this in principle cannot happen, since processUnconnectedNodes() is called before
  for(auto &module : getSubmoduleListRecursive()) {
    for(auto &accessor : module->getAccessorList()) {
      if(!accessor.hasOwner()) {
        throw std::invalid_argument("The accessor '"+accessor.getName()+"' of the module '"+module->getName()+    // LCOV_EXCL_LINE
                                    "' was not connected!");                                                      // LCOV_EXCL_LINE
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

  // prepare the modules
  for(auto &module : getSubmoduleListRecursive()) {
    module->prepare();
  }

  // start the necessary threads for the FanOuts etc.
  for(auto &internalModule : internalModuleList) {
    internalModule->activate();
  }

  // read all input variables once, to set the startup value e.g. coming from the config file
  // (without triggering an action inside the application)
  for(auto &module : getSubmoduleListRecursive()) {
    for(auto &variable : module->getAccessorList()) {
      if(variable.getDirection() == VariableDirection::consuming) {
        variable.getAppAccessorNoType().readLatest();
      }
    }
  }

  // start the threads for the modules
  for(auto &module : getSubmoduleListRecursive()) {
    module->run();
  }

}

/*********************************************************************************************************************/

void Application::shutdown() {
  
  // first allow to run the application threads again, if we are in testable mode
  if(testableMode && testableModeTestLock()) {
    testableModeUnlock("shutdown");
  }

  // deactivate the FanOuts first, since they have running threads inside accessing the modules etc.
  // (note: the modules are members of the Application implementation and thus get destroyed after this destructor)
  for(auto &internalModule : internalModuleList) {
    internalModule->deactivate();
  }

  // next deactivate the modules, as they have running threads inside as well
  for(auto &module : getSubmoduleListRecursive()) {
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
    if(&(a.getOwner()) != &(b.getOwner())) {      /// @todo TODO merge networks?
      std::cout << "*** ERROR: nodes to be connected should have the same owner!" << std::endl;
      std::cout << "Node A:" << std::endl;
      a.dump();
      std::cout << "Node B:" << std::endl;
      b.dump();
      std::cout << "Owner of node A:" << std::endl;
      a.getOwner().dump();
      std::cout << "Owner of node B:" << std::endl;
      b.getOwner().dump();
      assert(&(a.getOwner()) == &(b.getOwner()));
    }
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
    if(!deviceMap[deviceAlias]->isOpen()) deviceMap[deviceAlias]->open();
  }

  // use wait_for_new_data mode if push update mode was requested
  mtca4u::AccessModeFlags flags{};
  if(mode == UpdateMode::push && direction == VariableDirection::consuming) flags = {AccessMode::wait_for_new_data};

  // return the register accessor from the device
  return deviceMap[deviceAlias]->getRegisterAccessor<UserType>(registerName, nElements, 0, flags);
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

  // create the ProcessArray for the proper UserType
  auto pvar = _processVariableManager->createProcessArray<UserType>(dir, node.getPublicName(), node.getNumberOfElements(),
                                                                    node.getOwner().getUnit(), node.getOwner().getDescription());
  assert(pvar->getName() != "");

  // decorate the process variable if testable mode is enabled and this is the receiving end of the variable
  // don't decorate if the feeding side is a constant!
  // Also don't decorate, if the mode is polling. Instead flag the variable to be polling, so the TestFacility is aware of this.
  if(testableMode && node.getDirection() == VariableDirection::feeding && node.getType() != NodeType::Constant) {
    
    // The transfer mode of this process variable is considered to be polling, if only one consumer exists and this
    // consumer is polling. Reason: mulitple consumers will result in the use of a FanOut, so the communication up to
    // the FanOut will be push-type, even if all consumers are poll-type.
    /// @todo Check if this is true!
    auto mode = UpdateMode::push;
    if(node.getOwner().countConsumingNodes() == 1) {
      if(node.getOwner().getConsumingNodes().front().getMode() == UpdateMode::poll) mode = UpdateMode::poll;
    }
    
    if(mode != UpdateMode::poll) {
      auto pvarDec = boost::make_shared<TestDecoratorRegisterAccessor<UserType>>(pvar);
      testableMode_names[pvarDec->getUniqueId()] = "ControlSystem:"+node.getPublicName();
      return pvarDec;
    }
    else {
      testableMode_isPollMode[pvar->getUniqueId()] = true;
    }
  }
  
  // return the process variable
  return pvar;
}

/*********************************************************************************************************************/

template<typename UserType>
std::pair< boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>, boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> >
  Application::createApplicationVariable(VariableNetworkNode const &node) {
    
  // obtain the meta data
  size_t nElements = node.getNumberOfElements();
  std::string name = node.getName();
  assert(name != "");
  
  // create the ProcessArray for the proper UserType
  auto pvarPair = createSynchronizedProcessArray<UserType>(nElements, name);
  assert(pvarPair.first->getName() != "");
  assert(pvarPair.second->getName() != "");

  // decorate the process variable if testable mode is enabled and mode is push-type
  if(testableMode && node.getMode() == UpdateMode::push) {
    std::pair< boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>,
               boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> > pvarPairDec;
    pvarPairDec.first = boost::make_shared<TestDecoratorRegisterAccessor<UserType>>(pvarPair.first);
    pvarPairDec.second = boost::make_shared<TestDecoratorRegisterAccessor<UserType>>(pvarPair.second);
    
    // put the decorators into the list
    testableMode_names[pvarPair.first->getUniqueId()] = "Internal:"+node.getQualifiedName();
    
    return pvarPairDec;
  }
  
  // return the pair
  return pvarPair;
}

/*********************************************************************************************************************/

void Application::makeConnections() {
  
  // apply optimisations
  // note: checks may not be run before since sometimes networks may only be valid after optimisations
  optimiseConnections();
  
  // run checks
  checkConnections();

  // make the connections for all networks
  for(auto &network : networkList) {
    makeConnectionsForNetwork(network);
  }

}

/*********************************************************************************************************************/

void Application::optimiseConnections() {

  // list of iterators of networks to be removed from the networkList after the merge operation
  std::list<VariableNetwork*> deleteNetworks;
  
  // search for networks with the same feeder
  for(auto it1 = networkList.begin(); it1 != networkList.end(); ++it1) {
    for(auto it2 = it1; it2 != networkList.end(); ++it2) {
      if(it1 == it2) continue;
     
      auto feeder1 = it1->getFeedingNode();
      auto feeder2 = it2->getFeedingNode();
      
      // this optimisation is only necessary for device-type nodes, since application and control-system nodes will
      // automatically create merged networks when having the same feeder
      /// @todo check if this assumtion is true! control-system nodes can be created with different types, too!
      if(feeder1.getType() != NodeType::Device || feeder2.getType() != NodeType::Device) continue;
      
      // check if referrring to same register
      if(feeder1.getDeviceAlias() != feeder2.getDeviceAlias()) continue;
      if(feeder1.getRegisterName() != feeder2.getRegisterName()) continue;
      
      // check if directions are the same
      if(feeder1.getDirection() != feeder2.getDirection()) continue;
      
      // check if value types and number of elements are compatible
      if(feeder1.getValueType() != feeder2.getValueType()) continue;
      if(feeder1.getNumberOfElements() != feeder2.getNumberOfElements()) continue;
      
      // check if transfer mode is the same
      if(feeder1.getMode() != feeder2.getMode()) continue;
      
      // check if triggers are compatible, if present
      if(feeder1.hasExternalTrigger() != feeder2.hasExternalTrigger()) continue;
      if(feeder1.hasExternalTrigger()) {
        if(feeder1.getExternalTrigger() != feeder2.getExternalTrigger()) continue;
      }
      
      // everything should be compatible at this point: merge the networks. We will merge the network of the outer
      // loop into the network of the inner loop, since the network of the outer loop will not be found a second time
      // in the inner loop.
      for(auto consumer : it1->getConsumingNodes()) {
        consumer.clearOwner();
        it2->addNode(consumer);
      }
      
      // if trigger present, remove corresponding trigger receiver node from the trigger network
      if(feeder1.hasExternalTrigger()) {
        for(auto &itTrig : networkList) {
          if(itTrig.getFeedingNode() != feeder1.getExternalTrigger()) continue;
          itTrig.removeNodeToTrigger(it1->getFeedingNode());
        }
      }

      // schedule the outer loop network for deletion and stop processing it
      deleteNetworks.push_back(&(*it1));
      break;
    }
  }
  
  // remove networks from the network list
  for(auto net : deleteNetworks) {
    networkList.remove(*net);
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
  else if(network.getValueType() == typeid(int64_t)) {
    typedMakeConnection<int64_t>(network);
  }
  else if(network.getValueType() == typeid(uint64_t)) {
    typedMakeConnection<uint64_t>(network);
  }
  else if(network.getValueType() == typeid(float)) {
    typedMakeConnection<float>(network);
  }
  else if(network.getValueType() == typeid(double)) {
    typedMakeConnection<double>(network);
  }
  else if(network.getValueType() == typeid(std::string)) {
    typedMakeConnection<std::string>(network);
  }
  else {
    throw std::invalid_argument("Unknown value type.");
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
  bool constantFeeder = feeder.getType() == NodeType::Constant;

  // 1st case: the feeder requires a fixed implementation
  if(feeder.hasImplementation() && !constantFeeder) {

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
    else {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
    }

    // if we just have two nodes, directly connect them
    if(nNodes == 2 && !useExternalTrigger) {
      auto consumer = consumers.front();
      if(consumer.getType() == NodeType::Application) {
        consumer.getAppAccessor<UserType>().replace(feedingImpl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Device) {
        auto consumingImpl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
        // connect the Device with e.g. a ControlSystem node via a ThreadedFanOut
        auto fanOut = boost::make_shared<ThreadedFanOut<UserType>>(feedingImpl);
        fanOut->addSlave(consumingImpl);
        internalModuleList.push_back(fanOut);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto consumingImpl = createProcessVariable<UserType>(consumer);
        // connect the ControlSystem with e.g. a Device node via an ThreadedFanOut
        auto fanOut = boost::make_shared<ThreadedFanOut<UserType>>(feedingImpl);
        fanOut->addSlave(consumingImpl);
        internalModuleList.push_back(fanOut);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(feedingImpl);
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else { /* !(nNodes == 2 && !useExternalTrigger) */

      // create the right FanOut type
      boost::shared_ptr<FanOut<UserType>> fanOut;
      boost::shared_ptr<ConsumingFanOut<UserType>> consumingFanOut;
      if(useExternalTrigger) {
        // if external trigger is enabled, use externally triggered threaded FanOut
        auto triggerNode = feeder.getExternalTrigger();
        auto triggerFanOut = triggerMap[triggerNode.getUniqueId()];
        if(!triggerFanOut) {
          triggerFanOut = boost::make_shared<TriggerFanOut>(network.getExternalTriggerImpl());
          triggerMap[triggerNode.getUniqueId()] = triggerFanOut;
          internalModuleList.push_back(triggerFanOut);
        }
        fanOut = triggerFanOut->addNetwork(feedingImpl);
      }
      else if(useFeederTrigger) {
        // if the trigger is provided by the pushing feeder, use the treaded version of the FanOut to distribute
        // new values immediately to all consumers.
        auto threadedFanOut = boost::make_shared<ThreadedFanOut<UserType>>(feedingImpl);
        internalModuleList.push_back(threadedFanOut);
        fanOut = threadedFanOut;
      }
      else {
        if(!network.hasApplicationConsumer()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("No application node in the network but no trigger!");
        }
        consumingFanOut = boost::make_shared<ConsumingFanOut<UserType>>(feedingImpl);
        fanOut = consumingFanOut;
      }

      // In case we have one or more trigger receivers among our consumers, we produce exactly one application variable
      // for it. We never need more, since the distribution is done with a TriggerFanOut.
      bool usedTriggerReceiver{false};        // flag if we already have a trigger receiver
      auto triggerConnection = createApplicationVariable<UserType>(feeder);   // will get destroyed if not used

      // add all consumers to the FanOut
      for(auto &consumer : consumers) {
        if(consumer.getType() == NodeType::Application) {
          if(consumingFanOut && consumer.getMode() == UpdateMode::poll) {
            consumer.getAppAccessor<UserType>().replace(consumingFanOut);
            consumingFanOut.reset();
          }
          else {
            auto impls = createApplicationVariable<UserType>(consumer);
            fanOut->addSlave(impls.first);
            consumer.getAppAccessor<UserType>().replace(impls.second);
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
          if(!usedTriggerReceiver) fanOut->addSlave(triggerConnection.first);
          usedTriggerReceiver = true;
          consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(triggerConnection.second);
        }
        else {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
        }
      }
      connectionMade = true;
    }
  }
  // 2nd case: the feeder does not require a fixed implementation
  else if(!constantFeeder) {    /* !feeder.hasImplementation() */
    // we should be left with an application feeder node
    if(feeder.getType() != NodeType::Application) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
    }
    assert(!useExternalTrigger);
    // if we just have two nodes, directly connect them
    if(nNodes == 2) {
      auto consumer = consumers.front();
      if(consumer.getType() == NodeType::Application) {
        auto impls = createApplicationVariable<UserType>(consumer);
        feeder.getAppAccessor<UserType>().replace(impls.first);
        consumer.getAppAccessor<UserType>().replace(impls.second);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto impl = createProcessVariable<UserType>(consumer);
        feeder.getAppAccessor<UserType>().replace(impl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Device) {
        auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
        feeder.getAppAccessor<UserType>().replace(impl);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        consumer.dump();
        auto impls = createApplicationVariable<UserType>(consumer);
        feeder.getAppAccessor<UserType>().replace(impls.first);
        consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(impls.second);
        connectionMade = true;
      }
      else if(consumer.getType() == NodeType::Constant) {
        auto impl = consumer.getConstAccessor<UserType>();
        feeder.getAppAccessor<UserType>().replace(impl);
        connectionMade = true;
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    else {
      // create FanOut and use it as the feeder implementation
      auto fanOut = boost::make_shared<FeedingFanOut<UserType>>(feeder.getName(), feeder.getUnit(),
                                                                feeder.getDescription(), feeder.getNumberOfElements());
      feeder.getAppAccessor<UserType>().replace(fanOut);

      // In case we have one or more trigger receivers among our consumers, we produce exactly one application variable
      // for it. We never need more, since the distribution is done with a TriggerFanOut.
      bool usedTriggerReceiver{false};        // flag if we already have a trigger receiver
      auto triggerConnection = createApplicationVariable<UserType>(feeder);   // will get destroyed if not used

      for(auto &consumer : consumers) {
        if(consumer.getType() == NodeType::Application) {
          auto impls = createApplicationVariable<UserType>(consumer);
          fanOut->addSlave(impls.first);
          consumer.getAppAccessor<UserType>().replace(impls.second);
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
          if(!usedTriggerReceiver) fanOut->addSlave(triggerConnection.first);
          usedTriggerReceiver = true;
          consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(triggerConnection.second);
        }
        else {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
        }
      }
      connectionMade = true;
    }
  }
  else {    /* constantFeeder */
    assert(feeder.getType() == NodeType::Constant);
    auto feedingImpl = feeder.getConstAccessor<UserType>();
    assert(feedingImpl != nullptr);
  
    for(auto &consumer : consumers) {
      if(consumer.getType() == NodeType::Application) {
        consumer.getAppAccessor<UserType>().replace(feedingImpl);
      }
      else if(consumer.getType() == NodeType::ControlSystem) {
        auto impl = createProcessVariable<UserType>(consumer);
        impl->accessChannel(0) = feedingImpl->accessChannel(0);
        impl->write();
      }
      else if(consumer.getType() == NodeType::Device) {
        auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
            VariableDirection::feeding, consumer.getMode(), consumer.getNumberOfElements());
        impl->accessChannel(0) = feedingImpl->accessChannel(0);
        impl->write();
      }
      else if(consumer.getType() == NodeType::TriggerReceiver) {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Using constants as triggers is not supported!");
      }
      else {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>("Unexpected node type!");
      }
    }
    connectionMade = true;
    
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

/*********************************************************************************************************************/

void Application::stepApplication() {
  // testableMode_counter must be non-zero, otherwise there is no input for the application to process
  if(testableMode_counter == 0) {
    throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
        "Application::stepApplication() called despite no input was provided to the application to process!");
  }
  // let the application run until it has processed all data (i.e. the semaphore counter is 0) 
  size_t oldCounter = 0;
  while(testableMode_counter > 0) {
    if(enableDebugTestableMode && ( oldCounter != testableMode_counter) ) {
      std::cout << "Application::stepApplication(): testableMode_counter = " << testableMode_counter << std::endl;
      oldCounter = testableMode_counter;
    }
    testableModeUnlock("stepApplication");
    boost::this_thread::yield();
    testableModeLock("stepApplication");
  }
}

/*********************************************************************************************************************/

TransferElement::ID Application::readAny(std::list<std::reference_wrapper<TransferElement>> elementsToRead) {
  if(!Application::getInstance().testableMode) {
    return mtca4u::TransferElement::readAny(elementsToRead);
  }
  else {
    try {
      testableModeUnlock("readAny");
    }
    catch(std::system_error &e) {   // ignore operation not permitted errors, since they happen the first time (lock not yet owned)
      if(e.code() != std::errc::operation_not_permitted) throw;
    }
    auto ret = mtca4u::TransferElement::readAny(elementsToRead);
    assert(testableModeTestLock());  // lock is acquired inside readAny(), since TestDecoratorTransferFuture::wait() is called there.
    return ret;
  }
}

/*********************************************************************************************************************/

void Application::testableModeLock(const std::string& name) {
  // don't do anything if testable mode is not enabled
  if(!getInstance().testableMode) return;
  
  // debug output if enabled (also prevent spamming the same message)
  if(getInstance().enableDebugTestableMode && getInstance().testableMode_repeatingMutexOwner == 0) {
    std::cout << "Application::testableModeLock(): Thread " << threadName()
              << " tries to obtain lock for " << name << std::endl;
  }
  
  // if last lock was obtained repeatedly by the same thread, sleep a short time before obtaining the lock to give the
  // other threads a chance to get the lock first
  if(getInstance().testableMode_repeatingMutexOwner > 0) usleep(10000);
  
  // obtain the lock
  getTestableModeLockObject().lock();
  
  // check if the last owner of the mutex was this thread, which may be a hint that no other thread is waiting for the
  // lock
  if(getInstance().testableMode_lastMutexOwner == std::this_thread::get_id()) {
    // debug output if enabled
    if(getInstance().enableDebugTestableMode && getInstance().testableMode_repeatingMutexOwner == 0) {
      std::cout << "Application::testableModeLock(): Thread " << threadName()
                << " repeatedly obtained lock successfully for " << name << ". Further messages will be suppressed." << std::endl;
    }
    
    // increase counter for stall detection
    getInstance().testableMode_repeatingMutexOwner++;
    
    // detect stall: if the same thread got the mutex with no other thread obtaining it in between for one second, we
    // assume no other thread is able to process data at this time. The test should fail in this case
    if(getInstance().testableMode_repeatingMutexOwner > 1000) {
      // print an informative message first, which lists also all variables currently containing unread data.
      std::cout << "*** Tests are stalled due to data which has been sent but not received." << std::endl;
      std::cout << "    The following variables still contain unread values or had data loss due to a queue overflow:" << std::endl;
      for(auto &pair : Application::getInstance().testableMode_perVarCounter) {
        if(pair.second > 0) {
          std::cout << "    - " << Application::getInstance().testableMode_names[pair.first];
          // check if process variable still has data in the queue
          try {
            if(getInstance().testableMode_processVars[pair.first]->readNonBlocking()) {
              std::cout << " (unread data in queue)";
            }
            else {
              std::cout << " (data loss)";
            }
          }
          catch(std::logic_error &e) {
            // if we receive a logic_error in readNonBlocking() it just means another thread is waiting on a
            // TransferFuture of this variable, and we actually were not allowed to read...
            std::cout << " (data loss)";
          }
          std::cout << std::endl;
        }
      }
      // throw an exception which cannot be caught (other than with catch all). This makes sure that the tests fail
      // properly
      class TestsStalled {};
      throw TestsStalled();
    }
  }
  else {
    // last owner of the mutex was different: reset the counter and store the thread id
    getInstance().testableMode_repeatingMutexOwner = 0;
    getInstance().testableMode_lastMutexOwner = std::this_thread::get_id();
    
    // debug output if enabled
    if(getInstance().enableDebugTestableMode) {
      std::cout << "Application::testableModeLock(): Thread " << threadName()
                << " obtained lock successfully for " << name << std::endl;
    }
  }
}

/*********************************************************************************************************************/

void Application::testableModeUnlock(const std::string& name) {
  if(!getInstance().testableMode) return;
  if(getInstance().enableDebugTestableMode && (!getInstance().testableMode_repeatingMutexOwner
    || getInstance().testableMode_lastMutexOwner != std::this_thread::get_id())) {
    std::cout << "Application::testableModeUnlock(): Thread " << threadName()
              << " releases lock for " << name << std::endl;
  }
  getTestableModeLockObject().unlock();
}
