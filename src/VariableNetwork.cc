/*
 * VariableNetwork.cc
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#include <sstream>

#include <libxml++/libxml++.h>

#include "VariableNetwork.h"
#include "Application.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetwork::hasFeedingNode() const {
    auto n = std::count_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode n) {
          return n.getDirection() == VariableDirection::feeding;
        } );
    assert(n < 2);
    return n == 1;
  }

  /*********************************************************************************************************************/

  size_t VariableNetwork::countConsumingNodes() const {
    return nodeList.size() - (hasFeedingNode() ? 1 : 0);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addNode(VariableNetworkNode &a) {
    if(a.hasOwner()) {  // already in the network
      assert( &(a.getOwner()) == this );    /// @todo TODO merge networks?
      return;
    }

    // change owner of the node: erase from Application's unconnectedNodeList and set this as owner
    a.setOwner(this);

    // if node is feeding, save as feeder for this network
    if(a.getDirection() == VariableDirection::feeding) {
      // make sure we only have one feeding node per network
      if(hasFeedingNode()) {
        std::stringstream msg;
        msg << "Trying to add a feeding accessor to a network already having a feeding accessor." << std::endl;
        msg << "The network you were trying to add the new accessor to:" << std::endl;
        dump("", msg);
        msg << "The node you were trying to add:" << std::endl;
        a.dump(msg);
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
      }
      // force value type, engineering unit and description of the network if set in this feeding node
      if(a.getValueType() != typeid(AnyType)) valueType = &(a.getValueType());
      if(a.getUnit() != mtca4u::TransferElement::unitNotSet) engineeringUnit = a.getUnit();
      if(a.getDescription() != "") description = a.getDescription();
    }
    else {
      // update value type and engineering unit, if not yet set
      if(valueType == &typeid(AnyType)) valueType = &(a.getValueType());
      if(engineeringUnit == mtca4u::TransferElement::unitNotSet) engineeringUnit = a.getUnit();
      if(description == "") description = a.getDescription();
    }
    // add node to node list
    nodeList.push_back(a);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::dump(const std::string& linePrefix, std::ostream& stream) const {
    stream << linePrefix << "VariableNetwork";
    stream << " [ptr: " << this << "]";
    stream << " {" << std::endl;
    stream << linePrefix << "  value type = " << valueType->name() << ", engineering unit = " << engineeringUnit << std::endl;
    stream << linePrefix << "  trigger type = ";
    try {
      TriggerType tt = getTriggerType(false);
      if(tt == TriggerType::feeder) stream << "feeder" << std::endl;
      if(tt == TriggerType::pollingConsumer) stream << "pollingConsumer" << std::endl;
      if(tt == TriggerType::external) stream << "external" << std::endl;
      if(tt == TriggerType::none) stream << "none" << std::endl;
    }
    catch(ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork> &e) {
      stream << "**error**" << std::endl;
    }
    stream << linePrefix << "  feeder";
    if(hasFeedingNode()) {
      getFeedingNode().dump(stream);
    }
    else {
      stream << " **error, no feeder found**" << std::endl;
    }
    stream << linePrefix << "  consumers: " << countConsumingNodes() << std::endl;
    size_t count = 0;
    for(auto &consumer : nodeList) {
      if(consumer.getDirection() != VariableDirection::consuming) continue;
      stream << linePrefix << "    # " << ++count << ":";
      consumer.dump(stream);
    }
    if(getFeedingNode().hasExternalTrigger()) {
      stream << linePrefix << "  external trigger node: ";
      getFeedingNode().getExternalTrigger().dump(stream);
    }
    stream << linePrefix << "}" << std::endl;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addNodeToTrigger(VariableNetworkNode& nodeToTrigger) {
    VariableNetworkNode node(nodeToTrigger, 0);
    node.setOwner(this);
    nodeList.push_back(node);
  }

  /*********************************************************************************************************************/

  VariableNetwork::TriggerType VariableNetwork::getTriggerType(bool verboseExceptions) const {
    const auto &feeder = getFeedingNode();
    // network has an external trigger
    if(feeder.hasExternalTrigger()) {
      if(feeder.getMode() == UpdateMode::push) {
        std::stringstream msg;
        msg << "Providing an external trigger to a variable network which is fed by a pushing variable is not allowed." << std::endl;
        if(verboseExceptions) {
          msg << "The illegal network:" << std::endl;
          dump("", msg);
        }
        throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
      }
      return TriggerType::external;
    }
    // network is fed by a pushing node
    if(feeder.getMode() == UpdateMode::push) {
      return TriggerType::feeder;
    }
    // network is fed by a poll-type node: must have exactly one polling consumer
    size_t nPollingConsumers = count_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode &n) {
          return n.getDirection() == VariableDirection::consuming && n.getMode() == UpdateMode::poll;
        } );
    if(nPollingConsumers != 1) {
      std::stringstream msg;
      msg << "In a network with a poll-type feeder and no external trigger, there must be exactly one polling consumer. Maybe you forgot to add a trigger?" << std::endl;
      if(verboseExceptions) {
        msg << "The illegal network:" << std::endl;
        dump("", msg);
      }
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
    }
    return TriggerType::pollingConsumer;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::check() {
    // must have consuming nodes
    if(countConsumingNodes() == 0) {
      std::stringstream msg;
      msg << "No consuming nodes connected to this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
    }

    // must have a feeding node
    if(!hasFeedingNode()) {
      std::stringstream msg;
      msg << "No feeding node connected to this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
    }

    // the network's value type must be correctly set
    if(*valueType == typeid(AnyType)) {
      std::stringstream msg;
      msg << "No data type specified for any of the nodes in this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
    }
    
    // the feeder node must have a non-zero length
    size_t length = getFeedingNode().getNumberOfElements();
    if(length == 0) {
      std::stringstream msg;
      msg << "The feeding node has zero (or undefined) length!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
    }
    
    // all consumers must have the same length as the feeder or a zero length for trigger receivers
    for(auto &node : nodeList) {
      if(node.getType() != NodeType::TriggerReceiver) {
        if(node.getNumberOfElements() != length) {
          std::stringstream msg;
          msg << "The network contains a node with a different length than the feeding node!" << std::endl;
          msg << "The illegal network:" << std::endl;
          dump("", msg);
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
        }
      }
      else {
        if(node.getNumberOfElements() != 0) {
          std::stringstream msg;
          msg << "The network contains a trigger receiver node with a non-zero length!" << std::endl;
          msg << "The illegal network:" << std::endl;
          dump("", msg);
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(msg.str());
        }
      }
    }

    // all nodes must have this network as the owner and a value type equal the network's value type
    for(auto &node : nodeList) {
      assert(&(node.getOwner()) == this);
      if(node.getValueType() == typeid(AnyType)) node.setValueType(*valueType);
      assert(node.getValueType() == *valueType);
    }

    // if the feeder is an application node, it must be in push mode
    if(getFeedingNode().getType() == NodeType::Application) {
      assert(getFeedingNode().getMode() == UpdateMode::push);
    }

    // check if trigger is correctly defined (the return type doesn't matter, only the checks done in the function are needed)
    getTriggerType();
  }

  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetwork::getFeedingNode() const {
    auto iter = std::find_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode n) {
          return n.getDirection() == VariableDirection::feeding;
        } );
    if(iter == nodeList.end()) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "No feeding node in this network!");
    }
    return *iter;
  }

  /*********************************************************************************************************************/

  std::list<VariableNetworkNode> VariableNetwork::getConsumingNodes() const{
    std::list<VariableNetworkNode> consumers;
    for(auto &n : nodeList) if(n.getDirection() == VariableDirection::consuming) consumers.push_back(n);
    return consumers;
  }

  /*********************************************************************************************************************/
  
  bool VariableNetwork::hasApplicationConsumer() const {
    for(auto &n : nodeList) {
      if(n.getDirection() == VariableDirection::consuming && n.getType() == NodeType::Application) return true;
    }
    return false;
  }

}
