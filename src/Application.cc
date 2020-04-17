/*
 * Application.cc
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#include <exception>
#include <fstream>
#include <string>
#include <thread>

#include <boost/fusion/container/map.hpp>
#include <ChimeraTK/BackendFactory.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "ArrayAccessor.h"
#include "ConstantAccessor.h"
#include "ConsumingFanOut.h"
#include "DebugPrintAccessorDecorator.h"
#include "DeviceModule.h"
#include "FeedingFanOut.h"
#include "ScalarAccessor.h"
#include "TestableModeAccessorDecorator.h"
#include "ThreadedFanOut.h"
#include "TriggerFanOut.h"
#include "VariableNetworkGraphDumpingVisitor.h"
#include "VariableNetworkNode.h"
#include "Visitor.h"
#include "XMLGeneratorVisitor.h"
#include "ExceptionHandlingDecorator.h"

using namespace ChimeraTK;

std::mutex Application::testableMode_mutex;

/*********************************************************************************************************************/

Application::Application(const std::string& name) : ApplicationBase(name), EntityOwner(name, "") {
  // check if the application name has been set
  if(applicationName == "") {
    shutdown();
    throw ChimeraTK::logic_error("Error: An instance of Application must have its applicationName set.");
  }
  // check if application name contains illegal characters
  std::string legalChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_";
  bool nameContainsIllegalChars = name.find_first_not_of(legalChars) != std::string::npos;
  if(nameContainsIllegalChars) {
    shutdown();
    throw ChimeraTK::logic_error("Error: The application name may only contain "
                                 "alphanumeric characters and underscores.");
  }
}

/*********************************************************************************************************************/

void Application::initialise() {
  if(initialiseCalled) {
    throw ChimeraTK::logic_error("Application::initialise() was already called before.");
  }

  // call the user-defined defineConnections() function which describes the
  // structure of the application
  defineConnections();

  // connect any unconnected accessors with constant values
  processUnconnectedNodes();

  // realise the connections between variable accessors as described in the
  // initialise() function
  makeConnections();

  // set flag to prevent further calls to this function and to prevent definition of additional connections.
  initialiseCalled = true;
}

/*********************************************************************************************************************/

/** Functor class to create a constant for otherwise unconnected variables,
 * suitable for boost::fusion::for_each(). */
namespace {
  struct CreateConstantForUnconnectedVar {
    /// @todo test unconnected variables for all types!
    CreateConstantForUnconnectedVar(const std::type_info& typeInfo, bool makeFeeder, size_t length)
    : _typeInfo(typeInfo), _makeFeeder(makeFeeder), _length(length) {}

    template<typename PAIR>
    void operator()(PAIR&) const {
      if(typeid(typename PAIR::first_type) != _typeInfo) return;
      theNode = VariableNetworkNode::makeConstant<typename PAIR::first_type>(
          _makeFeeder, typename PAIR::first_type(), _length);
      done = true;
    }

    const std::type_info& _typeInfo;
    bool _makeFeeder;
    size_t _length;
    mutable bool done{false};
    mutable VariableNetworkNode theNode;
  };
} // namespace

/*********************************************************************************************************************/

void Application::processUnconnectedNodes() {
  for(auto& module : getSubmoduleListRecursive()) {
    for(auto& accessor : module->getAccessorList()) {
      if(!accessor.hasOwner()) {
        if(enableUnconnectedVariablesWarning) {
          std::cerr << "*** Warning: Variable '" << accessor.getQualifiedName()
                    << "' is not connected. " // LCOV_EXCL_LINE
                       "Reading will always result in 0, writing will be ignored."
                    << std::endl; // LCOV_EXCL_LINE
        }
        networkList.emplace_back();
        networkList.back().addNode(accessor);

        bool makeFeeder = !(networkList.back().hasFeedingNode());
        size_t length = accessor.getNumberOfElements();
        auto callable = CreateConstantForUnconnectedVar(accessor.getValueType(), makeFeeder, length);
        boost::fusion::for_each(ChimeraTK::userTypeMap(), std::ref(callable));
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
  for(auto& network : networkList) {
    network.check();
  }

  // check if all accessors are connected
  // note: this in principle cannot happen, since processUnconnectedNodes() is
  // called before
  for(auto& module : getSubmoduleListRecursive()) {
    for(auto& accessor : module->getAccessorList()) {
      if(!accessor.hasOwner()) {
        throw ChimeraTK::logic_error("The accessor '" + accessor.getName() + "' of the module '" +
            module->getName() +      // LCOV_EXCL_LINE
            "' was not connected!"); // LCOV_EXCL_LINE
      }
    }
  }
}

/*********************************************************************************************************************/

void Application::run() {
  assert(applicationName != "");

  if(testableMode) {
    if(!testFacilityRunApplicationCalled) {
      throw ChimeraTK::logic_error(
          "Testable mode enabled but Application::run() called directly. Call TestFacility::runApplication() instead.");
    }
  }

  if(runCalled) {
    throw ChimeraTK::logic_error("Application::run() has already been called before.");
  }
  runCalled = true;

  // set all initial version numbers in the modules to the same value
  for(auto& module : getSubmoduleListRecursive()) {
    if(module->getModuleType() != ModuleType::ApplicationModule) continue;
    module->setCurrentVersionNumber(getStartVersion());
  }

  // prepare the modules
  for(auto& module : getSubmoduleListRecursive()) {
    module->prepare();
  }
  for(auto& deviceModule : deviceModuleList) {
    deviceModule->prepare();
  }

  // check for application PVs which have a value, which needs to be propagated as initial value
  for(auto& module : getSubmoduleListRecursive()) {
    for(auto& var : module->getAccessorList()) {
      if(!var.getAppAccessorNoType().isWriteable()) continue;
      if(var.getAppAccessorNoType().getVersionNumber() >= getStartVersion()) {
        var.setHasInitialValue(true);
      }
    }
  }

  // Switch life-cycle state to run
  lifeCycleState = LifeCycleState::run;

  // start the necessary threads for the FanOuts etc.
  for(auto& internalModule : internalModuleList) {
    internalModule->activate();
  }

  for(auto& deviceModule : deviceModuleList) {
    deviceModule->run();
  }

  // start the threads for the modules
  for(auto& module : getSubmoduleListRecursive()) {
    module->run();
  }
}

/*********************************************************************************************************************/

void Application::shutdown() {
  // switch life-cycle state
  lifeCycleState = LifeCycleState::shutdown;

  // first allow to run the application threads again, if we are in testable
  // mode
  if(testableMode && testableModeTestLock()) {
    testableModeUnlock("shutdown");
  }

  // deactivate the FanOuts first, since they have running threads inside
  // accessing the modules etc. (note: the modules are members of the
  // Application implementation and thus get destroyed after this destructor)
  for(auto& internalModule : internalModuleList) {
    internalModule->deactivate();
  }

  // next deactivate the modules, as they have running threads inside as well
  for(auto& module : getSubmoduleListRecursive()) {
    module->terminate();
  }

  for(auto& deviceModule : deviceModuleList) {
    deviceModule->terminate();
  }
  ApplicationBase::shutdown();
}
/*********************************************************************************************************************/

void Application::generateXML() {
  assert(applicationName != "");

  // define the connections
  defineConnections();

  // also search for unconnected nodes - this is here only executed to print the
  // warnings
  processUnconnectedNodes();

  // create connections for exception handling
  for(auto& devModule : deviceModuleList) {
    devModule->defineConnections();
  }

  // finalise connections: decide still-undecided details, in particular for
  // control-system and device varibales, which get created "on the fly".
  finaliseNetworks();

  XMLGeneratorVisitor visitor;
  visitor.dispatch(*this);
  visitor.save(applicationName + ".xml");
}

/*********************************************************************************************************************/

VariableNetwork& Application::connect(VariableNetworkNode a, VariableNetworkNode b) {
  // if one of the nodes has the value type AnyType, set it to the type of the
  // other if both are AnyType, nothing changes.
  if(a.getValueType() == typeid(AnyType)) {
    a.setValueType(b.getValueType());
  }
  else if(b.getValueType() == typeid(AnyType)) {
    b.setValueType(a.getValueType());
  }

  // if one of the nodes has not yet a defined number of elements, set it to the
  // number of elements of the other. if both are undefined, nothing changes.
  if(a.getNumberOfElements() == 0) {
    a.setNumberOfElements(b.getNumberOfElements());
  }
  else if(b.getNumberOfElements() == 0) {
    b.setNumberOfElements(a.getNumberOfElements());
  }
  if(a.getNumberOfElements() != b.getNumberOfElements()) {
    std::stringstream what;
    what << "*** ERROR: Cannot connect array variables with difference number "
            "of elements!"
         << std::endl;
    what << "Node A:" << std::endl;
    a.dump(what);
    what << "Node B:" << std::endl;
    b.dump(what);
    throw ChimeraTK::logic_error(what.str());
  }

  // if both nodes already have an owner, we are either already done (same
  // owners) or we need to try to merge the networks
  if(a.hasOwner() && b.hasOwner()) {
    if(&(a.getOwner()) != &(b.getOwner())) {
      auto& networkToMerge = b.getOwner();
      bool success = a.getOwner().merge(networkToMerge);
      if(!success) {
        std::stringstream what;
        what << "*** ERROR: Trying to connect two nodes which are already part "
                "of different networks, and merging these"
                " networks is not possible (cannot have two non-control-system "
                "or two control-system feeders)!"
             << std::endl;
        what << "Node A:" << std::endl;
        a.dump(what);
        what << "Node B:" << std::endl;
        b.dump(what);
        what << "Owner of node A:" << std::endl;
        a.getOwner().dump("", what);
        what << "Owner of node B:" << std::endl;
        b.getOwner().dump("", what);
        throw ChimeraTK::logic_error(what.str());
      }
      for(auto n = networkList.begin(); n != networkList.end(); ++n) {
        if(&*n == &networkToMerge) {
          networkList.erase(n);
          break;
        }
      }
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
boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> Application::createDeviceVariable(
    const std::string& deviceAlias, const std::string& registerName, VariableDirection direction, UpdateMode mode,
    size_t nElements) {
  // Device opens in DeviceModule
  if(deviceMap.count(deviceAlias) == 0) {
    deviceMap[deviceAlias] = ChimeraTK::BackendFactory::getInstance().createBackend(deviceAlias);
  }

  // use wait_for_new_data mode if push update mode was requested
  // Feeding to the network means reading from a device to feed it into the network.
  ChimeraTK::AccessModeFlags flags{};
  if(mode == UpdateMode::push && direction.dir == VariableDirection::feeding) flags = {AccessMode::wait_for_new_data};

  // obtain the register accessor from the device
  auto accessor = deviceMap[deviceAlias]->getRegisterAccessor<UserType>(registerName, nElements, 0, flags);

  // find the right DeviceModule for this alias name - required for exception handling
  DeviceModule* devmod = nullptr;
  for(auto& dm : deviceModuleList) {
    if(dm->deviceAliasOrURI == deviceAlias) {
      devmod = dm;
      break;
    }
  }
  assert(devmod != nullptr);

  // decorate the accessor with a ExceptionHandlingDecorator and return it
  // Consuming from the network means writing to the device what you consumed.
  if(direction.dir == VariableDirection::consuming) {
    // writable registers additionally get a recoveryAccessor
    // Notice: Don't use the fact that there is a recovery accessor to determine the direction in the ExceptionHandlingDecorator.
    // There will be write-accessors without recovery accessors in future (intentionally turned off by the application programmer)
    auto recoveryAccessor = deviceMap[deviceAlias]->getRegisterAccessor<UserType>(registerName, nElements, 0, flags);
    return boost::make_shared<ExceptionHandlingDecorator<UserType>>(accessor, *devmod, direction, recoveryAccessor);
  }
  else {
    return boost::make_shared<ExceptionHandlingDecorator<UserType>>(accessor, *devmod, direction);
  }
}

/*********************************************************************************************************************/

template<typename UserType>
boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> Application::createProcessVariable(
    VariableNetworkNode const& node) {
  // determine the SynchronizationDirection
  SynchronizationDirection dir;
  if(node.getDirection().withReturn) {
    dir = SynchronizationDirection::bidirectional;
  }
  else if(node.getDirection().dir == VariableDirection::feeding) {
    dir = SynchronizationDirection::controlSystemToDevice;
  }
  else {
    dir = SynchronizationDirection::deviceToControlSystem;
  }
  AccessModeFlags flags = {};
  if(node.getDirection().dir == VariableDirection::consuming) { // Application-to-controlsystem must be
                                                                // push-type
    flags = {AccessMode::wait_for_new_data};
  }
  else {
    for(auto& consumer : node.getOwner().getConsumingNodes()) {
      if(consumer.getMode() == UpdateMode::push) flags = {AccessMode::wait_for_new_data};
    }
  }

  // create the ProcessArray for the proper UserType
  auto pvar = _processVariableManager->createProcessArray<UserType>(dir, node.getPublicName(),
      node.getNumberOfElements(), node.getOwner().getUnit(), node.getOwner().getDescription(), {}, 3, flags);
  assert(pvar->getName() != "");

  // create variable ID
  auto varId = getNextVariableId();
  pvIdMap[pvar->getUniqueId()] = varId;

  // Decorate the process variable if testable mode is enabled and this is the
  // receiving end of the variable (feeding to the network). Also don't decorate, if the mode is polling.
  // Instead flag the variable to be polling, so the TestFacility is aware of
  // this.
  if(testableMode && node.getDirection().dir == VariableDirection::feeding) {
    // The transfer mode of this process variable is considered to be polling,
    // if only one consumer exists and this consumer is polling. Reason:
    // mulitple consumers will result in the use of a FanOut, so the
    // communication up to the FanOut will be push-type, even if all consumers
    // are poll-type.
    /// @todo Check if this is true!
    auto mode = UpdateMode::push;
    if(node.getOwner().countConsumingNodes() == 1) {
      if(node.getOwner().getConsumingNodes().front().getMode() == UpdateMode::poll) mode = UpdateMode::poll;
    }

    if(mode != UpdateMode::poll) {
      auto pvarDec = boost::make_shared<TestableModeAccessorDecorator<UserType>>(pvar, true, false, varId, varId);
      testableMode_names[varId] = "ControlSystem:" + node.getPublicName();
      return pvarDec;
    }
    else {
      testableMode_isPollMode[varId] = true;
    }
  }

  // return the process variable
  return pvar;
}

/*********************************************************************************************************************/

template<typename UserType>
std::pair<boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>,
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>>
    Application::createApplicationVariable(VariableNetworkNode const& node, VariableNetworkNode const& consumer) {
  // obtain the meta data
  size_t nElements = node.getNumberOfElements();
  std::string name = node.getName();
  assert(name != "");
  AccessModeFlags flags = {};
  if(consumer.getType() != NodeType::invalid) {
    if(consumer.getMode() == UpdateMode::push) flags = {AccessMode::wait_for_new_data};
  }
  else {
    if(node.getMode() == UpdateMode::push) flags = {AccessMode::wait_for_new_data};
  }

  // create the ProcessArray for the proper UserType
  std::pair<boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>,
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>>
      pvarPair;
  if(consumer.getType() != NodeType::invalid)
    assert(node.getDirection().withReturn == consumer.getDirection().withReturn);
  if(!node.getDirection().withReturn) {
    pvarPair = createSynchronizedProcessArray<UserType>(
        nElements, name, node.getUnit(), node.getDescription(), {}, 3, {}, flags);
  }
  else {
    pvarPair = createBidirectionalSynchronizedProcessArray<UserType>(
        nElements, name, node.getUnit(), node.getDescription(), {}, 3, {}, {}, flags);
  }
  assert(pvarPair.first->getName() != "");
  assert(pvarPair.second->getName() != "");

  // create variable IDs
  size_t varId = getNextVariableId();
  size_t varIdReturn;
  if(node.getDirection().withReturn) varIdReturn = getNextVariableId();

  // decorate the process variable if testable mode is enabled and mode is
  // push-type
  if(testableMode && node.getMode() == UpdateMode::push) {
    if(!node.getDirection().withReturn) {
      pvarPair.first =
          boost::make_shared<TestableModeAccessorDecorator<UserType>>(pvarPair.first, false, true, varId, varId);
      pvarPair.second =
          boost::make_shared<TestableModeAccessorDecorator<UserType>>(pvarPair.second, true, false, varId, varId);
    }
    else {
      pvarPair.first =
          boost::make_shared<TestableModeAccessorDecorator<UserType>>(pvarPair.first, true, true, varIdReturn, varId);
      pvarPair.second =
          boost::make_shared<TestableModeAccessorDecorator<UserType>>(pvarPair.second, true, true, varId, varIdReturn);
    }

    // put the decorators into the list
    testableMode_names[varId] = "Internal:" + node.getQualifiedName();
    if(consumer.getType() != NodeType::invalid) {
      testableMode_names[varId] += "->" + consumer.getQualifiedName();
    }
    if(node.getDirection().withReturn) testableMode_names[varIdReturn] = testableMode_names[varId] + " (return)";
  }

  // if debug mode was requested for either node, decorate both accessors
  if(debugMode_variableList.count(node.getUniqueId()) ||
      (consumer.getType() != NodeType::invalid && debugMode_variableList.count(consumer.getUniqueId()))) {
    if(consumer.getType() != NodeType::invalid) {
      assert(node.getDirection().dir == VariableDirection::feeding);
      assert(consumer.getDirection().dir == VariableDirection::consuming);
      pvarPair.first =
          boost::make_shared<DebugPrintAccessorDecorator<UserType>>(pvarPair.first, node.getQualifiedName());
      pvarPair.second =
          boost::make_shared<DebugPrintAccessorDecorator<UserType>>(pvarPair.second, consumer.getQualifiedName());
    }
    else {
      pvarPair.first =
          boost::make_shared<DebugPrintAccessorDecorator<UserType>>(pvarPair.first, node.getQualifiedName());
      pvarPair.second =
          boost::make_shared<DebugPrintAccessorDecorator<UserType>>(pvarPair.second, node.getQualifiedName());
    }
  }

  // return the pair
  return pvarPair;
}

/*********************************************************************************************************************/

void Application::makeConnections() {
  for(auto& devModule : deviceModuleList) {
    devModule->defineConnections();
  }
  // finalise connections: decide still-undecided details, in particular for
  // control-system and device varibales, which get created "on the fly".
  finaliseNetworks();

  // apply optimisations
  // note: checks may not be run before since sometimes networks may only be
  // valid after optimisations
  optimiseConnections();

  // run checks
  checkConnections();

  // make the connections for all networks
  for(auto& network : networkList) {
    makeConnectionsForNetwork(network);
  }
}

/*********************************************************************************************************************/

void Application::finaliseNetworks() {
  // check for control system variables which should be made bidirectional
  for(auto& network : networkList) {
    size_t nBidir = network.getFeedingNode().getDirection().withReturn ? 1 : 0;
    for(auto& consumer : network.getConsumingNodes()) {
      if(consumer.getDirection().withReturn) ++nBidir;
    }
    if(nBidir != 1)
      continue; // only if there is exactly one node with return channel we need
                // to guess its peer
    if(network.getFeedingNode().getType() != NodeType::ControlSystem) {
      // only a feeding control system variable can be made bidirectional
      continue;
    }
    network.getFeedingNode().setDirection({VariableDirection::feeding, true});
  }

  // check for networks which have an external trigger but should be triggered by pollling consumer
  for(auto& network : networkList) {
    if(network.getTriggerType() == VariableNetwork::TriggerType::external) {
      size_t pollingComsumers{0};
      for(auto& consumer : network.getConsumingNodes()) {
        if(consumer.getMode() == UpdateMode::poll) ++pollingComsumers;
      }
      if(pollingComsumers == 1) {
        network.getFeedingNode().removeExternalTrigger();
      }
    }
  }
}

/*********************************************************************************************************************/

void Application::optimiseConnections() {
  // list of iterators of networks to be removed from the networkList after the
  // merge operation
  std::list<VariableNetwork*> deleteNetworks;

  // search for networks with the same feeder
  for(auto it1 = networkList.begin(); it1 != networkList.end(); ++it1) {
    for(auto it2 = it1; it2 != networkList.end(); ++it2) {
      if(it1 == it2) continue;

      auto feeder1 = it1->getFeedingNode();
      auto feeder2 = it2->getFeedingNode();

      // this optimisation is only necessary for device-type nodes, since
      // application and control-system nodes will automatically create merged
      // networks when having the same feeder
      /// @todo check if this assumtion is true! control-system nodes can be
      /// created with different types, too!
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

      // everything should be compatible at this point: merge the networks. We
      // will merge the network of the outer loop into the network of the inner
      // loop, since the network of the outer loop will not be found a second
      // time in the inner loop.
      for(auto consumer : it1->getConsumingNodes()) {
        consumer.clearOwner();
        it2->addNode(consumer);
      }

      // if trigger present, remove corresponding trigger receiver node from the
      // trigger network
      if(feeder1.hasExternalTrigger()) {
        feeder1.getExternalTrigger().getOwner().removeNodeToTrigger(it1->getFeedingNode());
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

void Application::dumpConnections(std::ostream& stream) {                                         // LCOV_EXCL_LINE
  stream << "==== List of all variable connections of the current Application ====" << std::endl; // LCOV_EXCL_LINE
  for(auto& network : networkList) {                                                              // LCOV_EXCL_LINE
    network.dump("", stream);                                                                     // LCOV_EXCL_LINE
  }                                                                                               // LCOV_EXCL_LINE
  stream << "=====================================================================" << std::endl; // LCOV_EXCL_LINE
} // LCOV_EXCL_LINE

void Application::dumpConnectionGraph(const std::string& fileName) {
  std::fstream file{fileName, std::ios_base::out};

  VariableNetworkGraphDumpingVisitor visitor{file};
  visitor.dispatch(*this);
}

/*********************************************************************************************************************/

Application::TypedMakeConnectionCaller::TypedMakeConnectionCaller(Application& owner, VariableNetwork& network)
: _owner(owner), _network(network) {}

/*********************************************************************************************************************/

template<typename PAIR>
void Application::TypedMakeConnectionCaller::operator()(PAIR&) const {
  if(typeid(typename PAIR::first_type) != _network.getValueType()) return;
  _owner.typedMakeConnection<typename PAIR::first_type>(_network);
  done = true;
}

/*********************************************************************************************************************/

void Application::makeConnectionsForNetwork(VariableNetwork& network) {
  // if the network has been created already, do nothing
  if(network.isCreated()) return;

  // if the trigger type is external, create the trigger first
  if(network.getFeedingNode().hasExternalTrigger()) {
    VariableNetwork& dependency = network.getFeedingNode().getExternalTrigger().getOwner();
    if(!dependency.isCreated()) makeConnectionsForNetwork(dependency);
  }

  // defer actual network creation to templated function
  auto callable = TypedMakeConnectionCaller(*this, network);
  boost::fusion::for_each(ChimeraTK::userTypeMap(), std::ref(callable));
  assert(callable.done);

  // mark the network as created
  network.markCreated();
}

/*********************************************************************************************************************/

template<typename UserType>
void Application::typedMakeConnection(VariableNetwork& network) {
  try {                          // catch exceptions to add information about the failed network
    bool connectionMade = false; // to check the logic...

    size_t nNodes = network.countConsumingNodes() + 1;
    auto feeder = network.getFeedingNode();
    auto consumers = network.getConsumingNodes();
    bool useExternalTrigger = network.getTriggerType() == VariableNetwork::TriggerType::external;
    bool useFeederTrigger = network.getTriggerType() == VariableNetwork::TriggerType::feeder;
    bool constantFeeder = feeder.getType() == NodeType::Constant;

    // 1st case: the feeder requires a fixed implementation
    if(feeder.hasImplementation() && !constantFeeder) {
      // Create feeding implementation. Note: though the implementation is derived
      // from the feeder, it will be used as the implementation of the (or one of
      // the) consumer. Logically, implementations are always pairs of
      // implementations (sender and receiver), but in this case the feeder
      // already has a fixed implementation pair. So our feedingImpl will contain
      // the consumer-end of the implementation pair. This is the reason why the
      // functions createProcessScalar() and createDeviceAccessor() get the
      // VariableDirection::consuming.
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl;
      if(feeder.getType() == NodeType::Device) {
        feedingImpl = createDeviceVariable<UserType>(feeder.getDeviceAlias(), feeder.getRegisterName(),
            {VariableDirection::feeding, false}, feeder.getMode(), feeder.getNumberOfElements());
      }
      else if(feeder.getType() == NodeType::ControlSystem) {
        feedingImpl = createProcessVariable<UserType>(feeder);
      }
      else {
        throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
      }

      // if we just have two nodes, directly connect them
      if(nNodes == 2 && !useExternalTrigger) {
        auto consumer = consumers.front();
        if(consumer.getType() == NodeType::Application) {
          consumer.setAppAccessorImplementation(feedingImpl);
          //check if the feedingImpl is from a device. In this case it has been decorated with an ExceptionHandlingDecorator
          auto feedingDeviceImpl =
              boost::dynamic_pointer_cast<ChimeraTK::ExceptionHandlingDecorator<UserType>>(feedingImpl);
          if(feedingDeviceImpl) {
            auto owningModule = consumer.getOwningModule(); // application module or variable group
            feedingDeviceImpl->setOwner(owningModule);
          }
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::Device) {
          auto consumingImpl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
              {VariableDirection::consuming, false}, consumer.getMode(), consumer.getNumberOfElements());
          // connect the Device with e.g. a ControlSystem node via a
          // ThreadedFanOut
          auto fanOut = boost::make_shared<ThreadedFanOut<UserType>>(feedingImpl, network);
          fanOut->addSlave(consumingImpl, consumer);
          internalModuleList.push_back(fanOut);
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto consumingImpl = createProcessVariable<UserType>(consumer);
          // connect the ControlSystem with e.g. a Device node via an
          // ThreadedFanOut
          auto fanOut = boost::make_shared<ThreadedFanOut<UserType>>(feedingImpl, network);
          fanOut->addSlave(consumingImpl, consumer);
          internalModuleList.push_back(fanOut);
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(feedingImpl);
          connectionMade = true;
        }
        else {
          throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
        }
      }
      else { /* !(nNodes == 2 && !useExternalTrigger) */

        // create the right FanOut type
        boost::shared_ptr<FanOut<UserType>> fanOut;
        boost::shared_ptr<ConsumingFanOut<UserType>> consumingFanOut;
        if(useExternalTrigger) {
          // if external trigger is enabled, use externally triggered threaded
          // FanOut. Create one per external trigger impl.
          void* triggerImplId = network.getExternalTriggerImpl().get();
          auto triggerFanOut = triggerMap[triggerImplId];
          if(!triggerFanOut) {
            // find the right DeviceModule for this alias name - required for exception handling
            DeviceModule* devmod = nullptr;
            std::string deviceAlias = feeder.getDeviceAlias();
            for(auto& dm : deviceModuleList) {
              if(dm->deviceAliasOrURI == deviceAlias) {
                devmod = dm;
                break;
              }
            }
            assert(devmod != nullptr);

            // create the trigger fan out and store it in the map and the internalModuleList
            triggerFanOut = boost::make_shared<TriggerFanOut>(network.getExternalTriggerImpl(), *devmod, network);
            triggerMap[triggerImplId] = triggerFanOut;
            internalModuleList.push_back(triggerFanOut);
          }
          fanOut = triggerFanOut->addNetwork(feedingImpl);
        }
        else if(useFeederTrigger) {
          // if the trigger is provided by the pushing feeder, use the treaded
          // version of the FanOut to distribute new values immediately to all
          // consumers. Depending on whether we have a return channel or not, pick
          // the right implementation of the FanOut
          boost::shared_ptr<ThreadedFanOut<UserType>> threadedFanOut;
          if(!feeder.getDirection().withReturn) {
            threadedFanOut = boost::make_shared<ThreadedFanOut<UserType>>(feedingImpl, network);
          }
          else {
            threadedFanOut = boost::make_shared<ThreadedFanOutWithReturn<UserType>>(feedingImpl, network);
          }
          internalModuleList.push_back(threadedFanOut);
          fanOut = threadedFanOut;
        }
        else {
          assert(network.hasApplicationConsumer()); // checkConnections should
                                                    // catch this
          consumingFanOut = boost::make_shared<ConsumingFanOut<UserType>>(feedingImpl);
          fanOut = consumingFanOut;
        }

        // In case we have one or more trigger receivers among our consumers, we
        // produce one consuming application variable for each device. Later this will create a TriggerFanOut for
        // each trigger consumer, i.e. one per device so one blocking device does not affect the others.
        /** Map of deviceAliases to their corresponding TriggerFanOuts. */
        std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>> triggerFanOuts;

        // add all consumers to the FanOut
        for(auto& consumer : consumers) {
          if(consumer.getType() == NodeType::Application) {
            if(consumingFanOut && consumer.getMode() == UpdateMode::poll) {
              consumer.setAppAccessorImplementation<UserType>(consumingFanOut);
              consumingFanOut.reset();
            }
            else {
              auto impls = createApplicationVariable<UserType>(consumer);
              fanOut->addSlave(impls.first, consumer);
              consumer.setAppAccessorImplementation<UserType>(impls.second);
            }
          }
          else if(consumer.getType() == NodeType::ControlSystem) {
            auto impl = createProcessVariable<UserType>(consumer);
            fanOut->addSlave(impl, consumer);
          }
          else if(consumer.getType() == NodeType::Device) {
            auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
                {VariableDirection::consuming, false}, consumer.getMode(), consumer.getNumberOfElements());
            fanOut->addSlave(impl, consumer);
          }
          else if(consumer.getType() == NodeType::TriggerReceiver) {
            std::string deviceAlias = consumer.getNodeToTrigger().getOwner().getFeedingNode().getDeviceAlias();
            auto triggerFanOut = triggerFanOuts[deviceAlias];
            if(!triggerFanOut) { // triggerFanOut is a shared pointer, which evaluates false if default constructed.
              // create a new process variable pair and set the sender/feeder to the fan out
              auto triggerConnection = createApplicationVariable<UserType>(feeder);
              triggerFanOut = triggerConnection.second;
              triggerFanOuts[deviceAlias] = triggerFanOut;
              fanOut->addSlave(triggerConnection.first, consumer);
            }
            consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(triggerFanOut);
          }
          else {
            throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
          }
        }
        connectionMade = true;
      }
    }
    // 2nd case: the feeder does not require a fixed implementation
    else if(!constantFeeder) { /* !feeder.hasImplementation() */
      // we should be left with an application feeder node
      if(feeder.getType() != NodeType::Application) {
        throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
      }
      assert(!useExternalTrigger);
      // if we just have two nodes, directly connect them
      if(nNodes == 2) {
        auto consumer = consumers.front();
        if(consumer.getType() == NodeType::Application) {
          auto impls = createApplicationVariable<UserType>(feeder, consumer);
          feeder.setAppAccessorImplementation<UserType>(impls.first);
          consumer.setAppAccessorImplementation<UserType>(impls.second);
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto impl = createProcessVariable<UserType>(consumer);
          feeder.setAppAccessorImplementation<UserType>(impl);
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::Device) {
          auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
              {VariableDirection::consuming, false}, consumer.getMode(), consumer.getNumberOfElements());
          feeder.setAppAccessorImplementation<UserType>(impl);
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          auto impls = createApplicationVariable<UserType>(feeder, consumer);
          feeder.setAppAccessorImplementation<UserType>(impls.first);
          consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(impls.second);
          connectionMade = true;
        }
        else if(consumer.getType() == NodeType::Constant) {
          auto impl = consumer.getConstAccessor<UserType>();
          feeder.setAppAccessorImplementation<UserType>(impl);
          connectionMade = true;
        }
        else {
          throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
        }
      }
      else {
        // create FanOut and use it as the feeder implementation
        auto fanOut = boost::make_shared<FeedingFanOut<UserType>>(feeder.getName(), feeder.getUnit(),
            feeder.getDescription(), feeder.getNumberOfElements(), feeder.getDirection().withReturn);
        feeder.setAppAccessorImplementation<UserType>(fanOut);

        // In case we have one or more trigger receivers among our consumers, we
        // produce one consuming application variable for each device. Later this will create a TriggerFanOut for
        // each trigger consimer, i.e. one per device so one blocking device does not affect the others.
        /** Map of deviceAliases to their corresponding TriggerFanOuts. */
        std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>> triggerFanOuts;

        for(auto& consumer : consumers) {
          if(consumer.getType() == NodeType::Application) {
            auto impls = createApplicationVariable<UserType>(consumer);
            fanOut->addSlave(impls.first, consumer);
            consumer.setAppAccessorImplementation<UserType>(impls.second);
          }
          else if(consumer.getType() == NodeType::ControlSystem) {
            auto impl = createProcessVariable<UserType>(consumer);
            fanOut->addSlave(impl, consumer);
          }
          else if(consumer.getType() == NodeType::Device) {
            auto impl = createDeviceVariable<UserType>(consumer.getDeviceAlias(), consumer.getRegisterName(),
                {VariableDirection::consuming, false}, consumer.getMode(), consumer.getNumberOfElements());
            fanOut->addSlave(impl, consumer);
          }
          else if(consumer.getType() == NodeType::TriggerReceiver) {
            std::string deviceAlias = consumer.getNodeToTrigger().getOwner().getFeedingNode().getDeviceAlias();
            auto triggerFanOut = triggerFanOuts[deviceAlias];
            if(!triggerFanOut) { // triggerFanOut is a shared pointer, which evaluates false if default constructed.
              // create a new process variable pair and set the sender/feeder to the fan out
              auto triggerConnection = createApplicationVariable<UserType>(feeder);
              triggerFanOut = triggerConnection.second;
              triggerFanOuts[deviceAlias] = triggerFanOut;
              fanOut->addSlave(triggerConnection.first, consumer);
            }
            consumer.getNodeToTrigger().getOwner().setExternalTriggerImpl(triggerFanOut);
          }
          else {
            throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
          }
        }
        connectionMade = true;
      }
    }
    else { /* constantFeeder */
      assert(feeder.getType() == NodeType::Constant);
      auto feedingImpl = feeder.getConstAccessor<UserType>();
      assert(feedingImpl != nullptr);

      for(auto& consumer : consumers) {
        if(consumer.getType() == NodeType::Application) {
          if(testableMode) {
            auto varId = getNextVariableId();
            auto pvarDec =
                boost::make_shared<TestableModeAccessorDecorator<UserType>>(feedingImpl, true, false, varId, varId);
            testableMode_names[varId] = "Constant";
            consumer.setAppAccessorImplementation<UserType>(pvarDec);
          }
          else {
            consumer.setAppAccessorImplementation<UserType>(feedingImpl);
          }
        }
        else if(consumer.getType() == NodeType::ControlSystem) {
          auto impl = createProcessVariable<UserType>(consumer);
          impl->accessChannel(0) = feedingImpl->accessChannel(0);
          impl->write();
        }
        else if(consumer.getType() == NodeType::Device) {
          // we register the required accessor as a recovery accessor. This is just a bare RegisterAccessor without any decorations directly from the backend.
          if(deviceMap.count(consumer.getDeviceAlias()) == 0) {
            deviceMap[consumer.getDeviceAlias()] =
                ChimeraTK::BackendFactory::getInstance().createBackend(consumer.getDeviceAlias());
          }
          auto impl = deviceMap[consumer.getDeviceAlias()]->getRegisterAccessor<UserType>(
              consumer.getRegisterName(), consumer.getNumberOfElements(), 0, {});
          impl->accessChannel(0) = feedingImpl->accessChannel(0);
          // find the right DeviceModule for this alias name
          DeviceModule* devmod = nullptr;
          for(auto& dm : deviceModuleList) {
            if(dm->deviceAliasOrURI == consumer.getDeviceAlias()) {
              devmod = dm;
              break;
            }
          }
          assert(devmod != nullptr);

          // The accessor implementation already has its data in the user buffer. We now just have to add a valid version number
          // and have a recovery accessors (RecoveryHelper to be excact) which we can register at the DeviceModule.
          // As this is a constant we don't need to change it later and don't have to store it somewere else.
          devmod->addRecoveryAccessor(boost::make_shared<RecoveryHelper>(impl, VersionNumber()));
        }
        else if(consumer.getType() == NodeType::TriggerReceiver) {
          throw ChimeraTK::logic_error("Using constants as triggers is not supported!");
        }
        else {
          throw ChimeraTK::logic_error("Unexpected node type!"); // LCOV_EXCL_LINE (assert-like)
        }
      }
      connectionMade = true;
    }

    if(!connectionMade) {                                                     // LCOV_EXCL_LINE (assert-like)
      throw ChimeraTK::logic_error(                                           // LCOV_EXCL_LINE (assert-like)
          "The variable network cannot be handled. Implementation missing!"); // LCOV_EXCL_LINE (assert-like)
    }                                                                         // LCOV_EXCL_LINE (assert-like)
  }
  catch(ChimeraTK::logic_error& e) {
    std::stringstream ss;
    ss << "ChimeraTK::logic_error thrown in Application::typedMakeConnection() for network:" << std::endl;
    network.dump("", ss);
    ss << e.what();
    throw ChimeraTK::logic_error(ss.str());
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

void Application::stepApplication(bool waitForDeviceInitialisation) {
  // testableMode_counter must be non-zero, otherwise there is no input for the application to process. It is also
  // sufficient if testableMode_deviceInitialisationCounter is non-zero, if waitForDeviceInitialisation == true. In that
  // case we only wait for the device initialisation to be completed.
  if(testableMode_counter == 0 && (!waitForDeviceInitialisation || testableMode_deviceInitialisationCounter == 0)) {
    throw ChimeraTK::logic_error("Application::stepApplication() called despite no input was provided "
                                 "to the application to process!");
  }
  // let the application run until it has processed all data (i.e. the semaphore
  // counter is 0)
  size_t oldCounter = 0;
  while(testableMode_counter > 0 || (waitForDeviceInitialisation && testableMode_deviceInitialisationCounter > 0)) {
    if(enableDebugTestableMode && (oldCounter != testableMode_counter)) { // LCOV_EXCL_LINE (only cout)
      std::cout << "Application::stepApplication(): testableMode_counter = " << testableMode_counter
                << std::endl;            // LCOV_EXCL_LINE (only cout)
      oldCounter = testableMode_counter; // LCOV_EXCL_LINE (only cout)
    }
    testableModeUnlock("stepApplication");
    boost::this_thread::yield();
    testableModeLock("stepApplication");
  }
}

/*********************************************************************************************************************/

void Application::testableModeLock(const std::string& name) {
  // don't do anything if testable mode is not enabled
  if(!getInstance().testableMode) return;

  // debug output if enabled (also prevent spamming the same message)
  if(getInstance().enableDebugTestableMode && getInstance().testableMode_repeatingMutexOwner == 0) { // LCOV_EXCL_LINE
                                                                                                     // (only cout)
    std::cout << "Application::testableModeLock(): Thread " << threadName() // LCOV_EXCL_LINE (only cout)
              << " tries to obtain lock for " << name << std::endl;         // LCOV_EXCL_LINE (only cout)
  }                                                                         // LCOV_EXCL_LINE (only cout)

  // if last lock was obtained repeatedly by the same thread, sleep a short time
  // before obtaining the lock to give the other threads a chance to get the
  // lock first
  if(getInstance().testableMode_repeatingMutexOwner > 0) usleep(10000);

  // obtain the lock
  getTestableModeLockObject().lock();

  // check if the last owner of the mutex was this thread, which may be a hint
  // that no other thread is waiting for the lock
  if(getInstance().testableMode_lastMutexOwner == std::this_thread::get_id()) {
    // debug output if enabled
    if(getInstance().enableDebugTestableMode && getInstance().testableMode_repeatingMutexOwner == 0) { // LCOV_EXCL_LINE
                                                                                                       // (only cout)
      std::cout << "Application::testableModeLock(): Thread " << threadName() // LCOV_EXCL_LINE (only cout)
                << " repeatedly obtained lock successfully for " << name      // LCOV_EXCL_LINE (only cout)
                << ". Further messages will be suppressed." << std::endl;     // LCOV_EXCL_LINE (only cout)
    }                                                                         // LCOV_EXCL_LINE (only cout)

    // increase counter for stall detection
    getInstance().testableMode_repeatingMutexOwner++;

    // detect stall: if the same thread got the mutex with no other thread
    // obtaining it in between for one second, we assume no other thread is able
    // to process data at this time. The test should fail in this case
    if(getInstance().testableMode_repeatingMutexOwner > 100) {
      // print an informative message first, which lists also all variables
      // currently containing unread data.
      std::cout << "*** Tests are stalled due to data which has been sent but "
                   "not received."
                << std::endl;
      std::cout << "    The following variables still contain unread values or "
                   "had data loss due to a queue overflow:"
                << std::endl;
      for(auto& pair : Application::getInstance().testableMode_perVarCounter) {
        if(pair.second > 0) {
          std::cout << "    - " << Application::getInstance().testableMode_names[pair.first] << " ["
                    << getInstance().testableMode_processVars[pair.first]->getId() << "]";
          // check if process variable still has data in the queue
          try {
            if(getInstance().testableMode_processVars[pair.first]->readNonBlocking()) {
              std::cout << " (unread data in queue)";
            }
            else {
              std::cout << " (data loss)";
            }
          }
          catch(std::logic_error&) {
            // if we receive a logic_error in readNonBlocking() it just means
            // another thread is waiting on a TransferFuture of this variable,
            // and we actually were not allowed to read...
            std::cout << " (data loss)";
          }
          std::cout << std::endl;
        }
      }
      std::cout << "(end of list)" << std::endl;
      // throw a specialised exception to make sure whoever catches it really
      // knows what he does...
      throw TestsStalled();
      // getInstance().testableMode_counter = 0;
      // for(auto &pair : Application::getInstance().testableMode_perVarCounter)
      // pair.second = 0;
    }
  }
  else {
    // last owner of the mutex was different: reset the counter and store the
    // thread id
    getInstance().testableMode_repeatingMutexOwner = 0;
    getInstance().testableMode_lastMutexOwner = std::this_thread::get_id();

    // debug output if enabled
    if(getInstance().enableDebugTestableMode) {                               // LCOV_EXCL_LINE (only cout)
      std::cout << "Application::testableModeLock(): Thread " << threadName() // LCOV_EXCL_LINE (only cout)
                << " obtained lock successfully for " << name << std::endl;   // LCOV_EXCL_LINE (only cout)
    }                                                                         // LCOV_EXCL_LINE (only cout)
  }
}

/*********************************************************************************************************************/

void Application::testableModeUnlock(const std::string& name) {
  if(!getInstance().testableMode) return;
  if(getInstance().enableDebugTestableMode &&
      (!getInstance().testableMode_repeatingMutexOwner                                   // LCOV_EXCL_LINE (only cout)
          || getInstance().testableMode_lastMutexOwner != std::this_thread::get_id())) { // LCOV_EXCL_LINE (only cout)
    std::cout << "Application::testableModeUnlock(): Thread " << threadName()            // LCOV_EXCL_LINE (only cout)
              << " releases lock for " << name << std::endl;                             // LCOV_EXCL_LINE (only cout)
  }                                                                                      // LCOV_EXCL_LINE (only cout)
  getTestableModeLockObject().unlock();
}
/*********************************************************************************************************************/

std::string& Application::threadName() {
  // Note: due to a presumed bug in gcc (still present in gcc 7), the
  // thread_local definition must be in the cc file to prevent seeing different
  // objects in the same thread under some conditions. Another workaround for
  // this problem can be found in commit
  // dc051bfe35ce6c1ed954010559186f63646cf5d4
  thread_local std::string name{"**UNNAMED**"};
  return name;
}

/*********************************************************************************************************************/

std::unique_lock<std::mutex>& Application::getTestableModeLockObject() {
  // Note: due to a presumed bug in gcc (still present in gcc 7), the
  // thread_local definition must be in the cc file to prevent seeing different
  // objects in the same thread under some conditions. Another workaround for
  // this problem can be found in commit
  // dc051bfe35ce6c1ed954010559186f63646cf5d4
  thread_local std::unique_lock<std::mutex> myLock(Application::testableMode_mutex, std::defer_lock);
  return myLock;
}

/*********************************************************************************************************************/
void Application::registerDeviceModule(DeviceModule* deviceModule) {
  deviceModuleList.push_back(deviceModule);
}

/*********************************************************************************************************************/
void Application::unregisterDeviceModule(DeviceModule* deviceModule) {
  deviceModuleList.remove(deviceModule);
}
