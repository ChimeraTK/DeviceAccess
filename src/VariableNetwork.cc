/*
 * VariableNetwork.cc
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#include <sstream>

#include "VariableNetwork.h"
#include "Application.h"
#include "VariableNetworkDumpingVisitor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetwork::hasFeedingNode() const {
    auto n = std::count_if( nodeList.begin(), nodeList.end(),
        [](const VariableNetworkNode node) {
          return node.getDirection() == VariableDirection::feeding;
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
    assert(!a.hasOwner());

    // change owner of the node: erase from Application's unconnectedNodeList and set this as owner
    a.setOwner(this);

    // if node is feeding, save as feeder for this network
    if(a.getDirection() == VariableDirection::feeding) {
      // make sure we only have one feeding node per network
      if(hasFeedingNode()) {
        // check if current feeding node is a control system variable: if yes, switch it to consuming
        if(getFeedingNode().getType() == NodeType::ControlSystem) {
          getFeedingNode().setDirection(VariableDirection::consuming);
        }
        // Current feeder cannot be switch to consumer: throw exception
        else{
          std::stringstream msg;
          msg << "Trying to add a feeding accessor to a network already having a feeding accessor." << std::endl;
          msg << "The network you were trying to add the new accessor to:" << std::endl;
          dump("", msg);
          msg << "The node you were trying to add:" << std::endl;
          a.dump(msg);
          throw ChimeraTK::logic_error(msg.str());
        }
      }
      // force value type, engineering unit and description of the network if set in this feeding node
      if(a.getValueType() != typeid(AnyType)) valueType = &(a.getValueType());
      if(a.getUnit() != ChimeraTK::TransferElement::unitNotSet) engineeringUnit = a.getUnit();
      if(a.getDescription() != "") description = a.getDescription();
    }
    else {
      // update value type and engineering unit, if not yet set
      if(valueType == &typeid(AnyType)) valueType = &(a.getValueType());
      if(engineeringUnit == ChimeraTK::TransferElement::unitNotSet) engineeringUnit = a.getUnit();
      if(description == "") description = a.getDescription();
    }
    // add node to node list
    nodeList.push_back(a);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::removeNode(VariableNetworkNode &a) {
    auto nNodes = nodeList.size();
    nodeList.remove(a);
    a.clearOwner();
    (void)nNodes;
    assert(nodeList.size() != nNodes);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::removeNodeToTrigger(const VariableNetworkNode &nodeToNoLongerTrigger) {
    for(auto &node : nodeList) {
      if(node.getType() != NodeType::TriggerReceiver) continue;
      if(node.getNodeToTrigger() == nodeToNoLongerTrigger) {
        removeNode(node);
        break;
      }
    }
  }

  /*********************************************************************************************************************/

  void VariableNetwork::dump(const std::string& linePrefix, std::ostream& stream) const {
      VariableNetworkDumpingVisitor visitor{linePrefix, stream};
      accept(visitor);
  }

  void VariableNetwork::accept(Visitor<VariableNetwork> &visitor) const {
      visitor.dispatch(*this);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addNodeToTrigger(VariableNetworkNode& nodeToTrigger) {
    VariableNetworkNode node(nodeToTrigger, 0);
    node.setOwner(this);
    nodeList.push_back(node);
  }

  /*********************************************************************************************************************/

  VariableNetwork::TriggerType VariableNetwork::getTriggerType(bool verboseExceptions) const {
    if(!hasFeedingNode()) return TriggerType::none;
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
        throw ChimeraTK::logic_error(msg.str());
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
      throw ChimeraTK::logic_error(msg.str());
    }
    return TriggerType::pollingConsumer;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::check() const {
    // must have consuming nodes
    if(countConsumingNodes() == 0) {
      std::stringstream msg;
      msg << "No consuming nodes connected to this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ChimeraTK::logic_error(msg.str());
    }

    // must have a feeding node
    if(!hasFeedingNode()) {
      std::stringstream msg;
      msg << "No feeding node connected to this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ChimeraTK::logic_error(msg.str());
    }

    // the network's value type must be correctly set
    if(*valueType == typeid(AnyType)) {
      std::stringstream msg;
      msg << "No data type specified for any of the nodes in this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ChimeraTK::logic_error(msg.str());
    }

    // the feeder node must have a non-zero length
    size_t length = getFeedingNode().getNumberOfElements();
    if(length == 0) {
      std::stringstream msg;
      msg << "The feeding node has zero (or undefined) length!" << std::endl;
      msg << "The illegal network:" << std::endl;
      dump("", msg);
      throw ChimeraTK::logic_error(msg.str());
    }

    // all consumers must have the same length as the feeder or a zero length for trigger receivers
    for(auto &node : nodeList) {
      if(node.getType() != NodeType::TriggerReceiver) {
        if(node.getNumberOfElements() != length) {
          std::stringstream msg;
          msg << "The network contains a node with a different length than the feeding node!" << std::endl;
          msg << "The illegal network:" << std::endl;
          dump("", msg);
          throw ChimeraTK::logic_error(msg.str());
        }
      }
      else {
        if(node.getNumberOfElements() != 0) {
          std::stringstream msg;
          msg << "The network contains a trigger receiver node with a non-zero length!" << std::endl;
          msg << "The illegal network:" << std::endl;
          dump("", msg);
          throw ChimeraTK::logic_error(msg.str());
        }
      }
    }

    // all nodes must have this network as the owner and a value type equal the network's value type
    for(auto &node : nodeList) {
      assert(&(node.getOwner()) == this);
      if(node.getValueType() == typeid(AnyType)) node.setValueType(*valueType);
      if(node.getValueType() != *valueType) {
        std::stringstream msg;
        msg << "The network contains variables of different value types, which is not supported!" << std::endl;
        msg << "The illegal network:" << std::endl;
        dump("", msg);
        throw ChimeraTK::logic_error(msg.str());
      }
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
      std::stringstream msg;
      msg << "No feeding node in this network!" << std::endl;
      msg << "The illegal network:" << std::endl;
      if(!hasFeedingNode()) dump("", msg);
      throw ChimeraTK::logic_error(msg.str());
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

  /*********************************************************************************************************************/

  bool VariableNetwork::merge(VariableNetwork &other) {

    // check if merging is possible
    if(hasFeedingNode() || !other.hasFeedingNode()) {
      if(    (getFeedingNode().getType() == NodeType::ControlSystem &&
              other.getFeedingNode().getType() == NodeType::ControlSystem)
          || (getFeedingNode().getType() != NodeType::ControlSystem &&
              other.getFeedingNode().getType() != NodeType::ControlSystem) ) {
        return false;
      }
    }

    // put all consuming nodes of B's owner into A's owner
    for(auto node : other.getConsumingNodes()) {
      other.removeNode(node);
      addNode(node);
    }

    // feeding node: if control system type, convert into consumer. Otherwise just add it, addNode() will convert the
    // other feeding node if necessary
    if(other.hasFeedingNode()) {
      VariableNetworkNode otherFeeder = other.getFeedingNode();
      other.removeNode(otherFeeder);
      if(otherFeeder.getType() == NodeType::ControlSystem) otherFeeder.setDirection(VariableDirection::consuming);
      addNode(otherFeeder);
    }

    return true;

  }
}
