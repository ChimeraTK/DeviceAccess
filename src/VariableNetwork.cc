/*
 * VariableNetwork.cc
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#include "VariableNetwork.h"
#include "Accessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetwork::hasAppNode(AccessorBase *a, AccessorBase *b) const {
    if(feeder.type == NodeType::Application) {
      if(a == feeder.appNode || (b != nullptr && b == feeder.appNode) ) return true;
    }

    // search for a and b in the inputAccessorList
    size_t c = count_if( consumerList.begin(), consumerList.end(),
        [a,b](const Node n) {
          return n.type == NodeType::Application && ( a == n.appNode || ( b != nullptr && b == n.appNode ) );
        } );

    if(c > 0) return true;
    return false;
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::hasFeedingNode() const {
    if(feeder.type == NodeType::Undefined) return false;
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
    Node node;
    node.type = NodeType::Application;
    node.mode = a.getUpdateMode();
    node.appNode = &a;
    if(a.isFeeding()) {
      if(hasFeedingNode()) {
        throw std::string("Trying to add an output accessor to a network already having an output accessor.");  // @todo TODO throw proper exception
      }
      feeder = node;
      valueType = &(a.getValueType());
    }
    else {
      consumerList.push_back(node);
    }
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addConsumingPublication(const std::string& name) {
    Node node;
    node.type = NodeType::ControlSystem;
    node.mode = UpdateMode::push;
    node.publicName = name;
    consumerList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingPublication(AccessorBase &a, const std::string& name) {
    if(hasFeedingNode()) {
      throw std::string("Trying to add control-system-to-device publication to a network already having an output accessor.");  // @todo TODO throw proper exception
    }
    feeder.type = NodeType::ControlSystem;
    feeder.mode = UpdateMode::push;
    feeder.publicName = name;
    valueType = &(a.getValueType());
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addConsumingDeviceRegister(const std::string &deviceAlias, const std::string &registerName,
      UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
    Node node;
    node.type = NodeType::Device;
    node.mode = mode;
    node.deviceAlias = deviceAlias;
    node.registerName = registerName;
    consumerList.push_back(node);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addFeedingDeviceRegister(AccessorBase &a, const std::string &deviceAlias,
      const std::string &registerName, UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
    if(hasFeedingNode()) {
      throw std::string("Trying to add control-system-to-device publication to a network already having an output accessor.");  // @todo TODO throw proper exception
    }
    feeder.type = NodeType::Device;
    feeder.mode = mode;
    feeder.deviceAlias = deviceAlias;
    feeder.registerName = registerName;
    valueType = &(a.getValueType());
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
}
