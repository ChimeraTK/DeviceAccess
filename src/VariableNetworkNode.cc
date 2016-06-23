/*
 * VariableNetworkNode.cc
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#include <libxml++/libxml++.h>

#include "VariableNetworkNode.h"
#include "VariableNetwork.h"
#include "Accessor.h"
#include "Application.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetworkNode::hasImplementation() const {
    return type == NodeType::Device || type == NodeType::ControlSystem;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::dump() const {
    if(type == NodeType::Application) std::cout << " type = Application";
    if(type == NodeType::ControlSystem) std::cout << " type = ControlSystem ('" << publicName << "')";
    if(type == NodeType::Device) std::cout << " type = Device (" << deviceAlias << ": " << registerName << ")";
    if(type == NodeType::TriggerReceiver) std::cout << " type = TriggerReceiver";
    if(type == NodeType::invalid) std::cout << " type = **invalid**";

    if(mode == UpdateMode::push) std::cout << " pushing" << std::endl;
    if(mode == UpdateMode::poll) std::cout << " polling" << std::endl;
}

  /*********************************************************************************************************************/

  void VariableNetworkNode::createXML(xmlpp::Element *rootElement) const {
    if(type != NodeType::ControlSystem) return;

    // Create the directory for the path name in the XML document with all parent directories, if not yet existing:
    // First split the publication name into components and loop over each component. For each component, try to find
    // the directory node and create it it does not exist. After the loop, the "current" will point to the Element
    // representing the directory.

    // strip the variable name from the path
    mtca4u::RegisterPath directory(publicName);
    directory--;

    // the namespace map is needed to properly refer to elements with an xpath expression in xmlpp::Element::find()
    /// @todo TODO move this somewhere else, or at least take the namespace URI from a common place!
    xmlpp::Node::PrefixNsMap nsMap{{"ac", "https://github.com/ChimeraTK/ApplicationCore"}};

    // go through each directory path component
    xmlpp::Element *current = rootElement;
    for(auto pathComponent : directory.getComponents()) {
      // find directory for this path component in the current directory
      std::string xpath = std::string("ac:directory[@name='")+pathComponent+std::string("']");
      auto list = current->find(xpath, nsMap);
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

  bool VariableNetworkNode::operator==(const VariableNetworkNode& other) const {
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

  bool VariableNetworkNode::operator!=(const VariableNetworkNode& other) const {
    return !operator==(other);
  }

}
