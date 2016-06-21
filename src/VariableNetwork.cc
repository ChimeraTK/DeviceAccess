/*
 * VariableNetwork.cc
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#include <libxml++/libxml++.h>

#include "VariableNetwork.h"
#include "Accessor.h"
#include "Application.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetwork::Node::hasImplementation() const {
    return type == NodeType::Device || type == NodeType::ControlSystem;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::Node::dump() const {
    if(type == NodeType::Application) std::cout << " type = Application" << std::endl;
    if(type == NodeType::ControlSystem) std::cout << " type = ControlSystem ('" << publicName << "')" << std::endl;
    if(type == NodeType::Device) std::cout << " type = Device (" << deviceAlias << ": "
        << registerName << ")" << std::endl;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::Node::createXML(xmlpp::Element *rootElement) const {
    if(type != NodeType::ControlSystem) return;

    // Create the directory for the path name in the XML document with all parent directories, if not yet existing:
    // First split the publication name into components and loop over each component. For each component, try to find
    // the directory node and create it it does not exist. After the loop, the "current" will point to the Element
    // representing the directory.

    // strip the variable name from the path
    mtca4u::RegisterPath directory(publicName);
    directory--;

    // go through each directory path component
    xmlpp::Element *current = rootElement;
    for(auto pathComponent : directory.getComponents()) {
      // find directory for this path component in the current directory
      auto list = current->find(std::string("/directory[@name=")+pathComponent+std::string("]"));
      if(list.size() == 0) {  // not found: create it
        xmlpp::Element *newChild = current->add_child("directory");
        newChild->set_attribute("name",pathComponent);
        current = newChild;
      }
      else {
        assert(list.size() == 1);
        current = dynamic_cast<xmlpp::Element*>(list[0]);
        assert(current != nullptr);
      }
    }

    // now add the variable to the directory
    xmlpp::Element *variable = current->add_child("variable");
    mtca4u::RegisterPath pathName(publicName);
    auto pathComponents = pathName.getComponents();

    // set the name attribute
    variable->set_attribute("name",pathComponents[pathComponents.size()-1]);

    // add sub-element containing the data type
    std::string dataTypeName{"unknown"};
    if(network->getValueType() == typeid(int8_t)) { dataTypeName = "int8"; }
    else if(network->getValueType() == typeid(uint8_t)) { dataTypeName = "uint8"; }
    else if(network->getValueType() == typeid(int16_t)) { dataTypeName = "int16"; }
    else if(network->getValueType() == typeid(uint16_t)) { dataTypeName = "uint16"; }
    else if(network->getValueType() == typeid(int32_t)) { dataTypeName = "int32"; }
    else if(network->getValueType() == typeid(uint32_t)) { dataTypeName = "uint32"; }
    else if(network->getValueType() == typeid(float)) { dataTypeName = "float"; }
    else if(network->getValueType() == typeid(double)) { dataTypeName = "double"; }
    else if(network->getValueType() == typeid(std::string)) { dataTypeName = "string"; }
    xmlpp::Element *valueTypeElement = variable->add_child("value_type");
    valueTypeElement->set_child_text(dataTypeName);

    // add sub-element containing the data flow direction
    std::string dataFlowName{"application_to_control_system"};
    if(network->getFeedingNode() == *this) { dataFlowName = "control_system_to_application"; }
    xmlpp::Element *directionElement = variable->add_child("direction");
    directionElement->set_child_text(dataFlowName);

    // add sub-element containing the engineering unit
    xmlpp::Element *unitElement = variable->add_child("unit");
    unitElement->set_child_text(network->getUnit());
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::Node::operator==(const Node& other) const {
    if(other.network != network) return false;
    if(other.type != type) return false;
    if(other.mode != mode) return false;
    if(other.appNode != appNode) return false;
    if(other.publicName != publicName) return false;
    if(other.deviceAlias != deviceAlias) return false;
    if(other.registerName != registerName) return false;
    return true;
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::Node::operator!=(const Node& other) const {
    return !operator==(other);
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::hasAppNode(AccessorBase *a, AccessorBase *b) const {
    if(feeder.type == NodeType::Application) {
      if(a == feeder.appNode || (b != nullptr && b == feeder.appNode) ) return true;
    }

    // search for a and b in the inputAccessorList
    size_t c = count_if( consumerList.begin(), consumerList.end(),
        [a,b](const Node n) {
          if(n.type != NodeType::Application) return false;
          return a == n.appNode || ( b != nullptr && b == n.appNode );
        } );

    if(c > 0) return true;
    return false;
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::hasFeedingNode() const {
    if(feeder.type == NodeType::invalid) return false;
    return true;
  }

  /*********************************************************************************************************************/

  size_t VariableNetwork::countConsumingNodes() const {
    return consumerList.size();
  }

  /*********************************************************************************************************************/

  size_t VariableNetwork::countFixedImplementations() const {
    size_t count = 0;
    if(feeder.hasImplementation()) count++;
    count += count_if( consumerList.begin(), consumerList.end(),
        [](const Node n) {
          return n.hasImplementation();
        } );
    return count;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addAppNode(AccessorBase &a) {
    if(hasAppNode(&a)) return;  // already in the network

    // create Node structure
    Node node;
    node.network = this;
    node.type = NodeType::Application;
    node.mode = a.getUpdateMode();
    node.appNode = &a;

    // if node is feeding, save as feeder for this network
    if(a.isFeeding()) {
      // make sure we only have one feeding node per network
      if(hasFeedingNode()) {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
            "Trying to add a feeding accessor to a network already having a feeding accessor.");
      }
      // update value type
      valueType = &(a.getValueType());
      // update engineering unit
      engineeringUnit = a.getUnit();
      // update feeder
      feeder = node;
    }
    // not is not feeding, add it to list of consumers
    else {
      // add node to consumer list
      consumerList.push_back(node);
    }
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addConsumingPublication(const std::string& name) {
    Node node;
    node.network = this;
    node.type = NodeType::ControlSystem;
    node.mode = UpdateMode::push;
    node.publicName = name;
    consumerList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingPublication(AccessorBase &a, const std::string& name) {
    addFeedingPublication(a.getValueType(), a.getUnit(), name);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingPublication(const std::type_info &typeInfo, const std::string& unit, const std::string& name) {
    if(hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Trying to add control-system-to-device publication to a network already having a feeding accessor.");
    }
    feeder.network = this;
    feeder.type = NodeType::ControlSystem;
    feeder.mode = UpdateMode::push;
    feeder.publicName = name;
    valueType = &typeInfo;
    engineeringUnit = unit;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addConsumingDeviceRegister(const std::string &deviceAlias, const std::string &registerName) {
    Node node;
    node.network = this;
    node.type = NodeType::Device;
    node.mode = UpdateMode::push;
    node.deviceAlias = deviceAlias;
    node.registerName = registerName;
    consumerList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingDeviceRegister(AccessorBase &a, const std::string &deviceAlias,
      const std::string &registerName, UpdateMode mode) {
    addFeedingDeviceRegister(a.getValueType(), a.getUnit(), deviceAlias, registerName, mode);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingDeviceRegister(const std::type_info &typeInfo, const std::string& unit
      , const std::string &deviceAlias, const std::string &registerName, UpdateMode mode) {
    if(hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Trying to add a feeding device register to a network already having a feeding accessor.");
    }
    feeder.network = this;
    feeder.type = NodeType::Device;
    feeder.mode = mode;
    feeder.deviceAlias = deviceAlias;
    feeder.registerName = registerName;
    valueType = &typeInfo;
    engineeringUnit = unit;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::dump() const {
    std::cout << "VariableNetwork {" << std::endl;
    std::cout << "  feeder";
    feeder.dump();
    std::cout << "  number of consumers: " << consumerList.size() << std::endl;
    size_t count = 0;
    for(auto &consumer : consumerList) {
      std::cout << "  consumer " << ++count << ":";
      consumer.dump();
    }
    std::cout << "}" << std::endl;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addTriggerReceiver(VariableNetwork *network) {
    Node node;
    node.network = this;
    node.type = NodeType::TriggerReceiver;
    node.mode = UpdateMode::push;
    node.triggerReceiver = network;
    consumerList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addTrigger(AccessorBase &trigger) {

    if(hasExternalTrigger) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Only one external trigger per variable network is allowed.");
    }

    // find the network the triggering accessor is connected to
    VariableNetwork &otherNetwork = Application::getInstance().findOrCreateNetwork(&trigger);
    otherNetwork.addAppNode(trigger);

    // add ourselves as a trigger receiver to the other network
    otherNetwork.addTriggerReceiver(this);

    // set flag and store pointer to other network
    hasExternalTrigger = true;
    externalTrigger = &otherNetwork;

  }

  /*********************************************************************************************************************/

  VariableNetwork::TriggerType VariableNetwork::getTriggerType() {
    // network has an external trigger
    if(hasExternalTrigger) {
      if(feeder.mode == UpdateMode::push) {
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
            "Providing an external trigger to a variable network which is fed by a pushing variable is not allowed.");
      }
      return TriggerType::external;
    }
    // network is fed by a pushing node
    if(feeder.mode == UpdateMode::push) {
      return TriggerType::feeder;
    }
    // network is fed by a poll-type node: must have exactly one polling consumer
    size_t nPollingConsumers = count_if( consumerList.begin(), consumerList.end(),
        [](const Node n) { return n.mode == UpdateMode::poll; } );
    if(nPollingConsumers != 1) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "In a network with a poll-type feeder and no external trigger, there must be exactly one polling consumer.");
    }
    return TriggerType::pollingConsumer;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::check() {
    // must have consuming nodes
    if(countConsumingNodes() == 0) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Illegal variable network found: no consuming nodes connected!");
    }

    // must have a feeding node
    if(!hasFeedingNode()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Illegal variable network found: no feeding node connected!");
    }

    // all nodes must have this network as the owner
    assert(feeder.network == this);
    for(auto &consumer : consumerList) {
      assert(consumer.network == this);
    }

    // check if trigger is correctly defined (the return type doesn't matter, only the checks done in the function are needed)
    getTriggerType();
  }

  /*********************************************************************************************************************/

  VariableNetwork& VariableNetwork::getExternalTrigger() {
    if(getTriggerType() != TriggerType::external) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
          "VariableNetwork::getExternalTrigger() may only be called if the trigger type is external.");
    }
    return *externalTrigger;
  }

}
