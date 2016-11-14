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

  VariableNetworkNode::VariableNetworkNode(const VariableNetworkNode &other)
  : pdata(other.pdata)
  {}

  /*********************************************************************************************************************/

  VariableNetworkNode& VariableNetworkNode::operator=(const VariableNetworkNode &rightHandSide) {
    pdata = rightHandSide.pdata;
    return *this;
  }


  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(AccessorBase &accessor)
  : pdata(new data)
  {
    pdata->type = NodeType::Application;
    pdata->mode = accessor.getUpdateMode();
    pdata->direction = accessor.getDirection();
    pdata->valueType = &(accessor.getValueType());
    pdata->unit = accessor.getUnit();
    pdata->appNode = &accessor;
    pdata->nElements = accessor.getNumberOfElements();
    pdata->description = accessor.getDescription();
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(const std::string &devAlias, const std::string &regName, UpdateMode mod,
      VariableDirection dir, const std::type_info &valTyp, size_t nElements)
  : pdata(new data)
  {
    pdata->type = NodeType::Device;
    pdata->mode = mod;
    pdata->direction = dir;
    pdata->valueType = &valTyp;
    pdata->deviceAlias = devAlias;
    pdata->registerName = regName;
    pdata->nElements = nElements;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(std::string pubName, VariableDirection dir, const std::type_info &valTyp,
      size_t nElements)
  : pdata(new data)
  {
    pdata->type = NodeType::ControlSystem;
    pdata->mode = UpdateMode::push;
    pdata->direction = dir;
    pdata->valueType = &valTyp;
    pdata->publicName = pubName;
    pdata->nElements = nElements;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(VariableNetworkNode& nodeToTrigger, int)
  : pdata(new data)
  {
    pdata->type = NodeType::TriggerReceiver;
    pdata->direction = VariableDirection::consuming;
    pdata->triggerReceiver = &nodeToTrigger;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setOwner(VariableNetwork *net) {
    assert(pdata->network == nullptr);
    assert(pdata->type != NodeType::invalid);
    pdata->network = net;
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::hasImplementation() const {
    return pdata->type == NodeType::Device || pdata->type == NodeType::ControlSystem || pdata->type == NodeType::Constant;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::dump() const {
    if(pdata->type == NodeType::Application) std::cout << " type = Application ('" << pdata->appNode->getName() << "')";
    if(pdata->type == NodeType::ControlSystem) std::cout << " type = ControlSystem ('" << pdata->publicName << "')";
    if(pdata->type == NodeType::Device) std::cout << " type = Device (" << pdata->deviceAlias << ": " << pdata->registerName << ")";
    if(pdata->type == NodeType::TriggerReceiver) std::cout << " type = TriggerReceiver";
    if(pdata->type == NodeType::Constant) std::cout << " type = Constant";
    if(pdata->type == NodeType::invalid) std::cout << " type = **invalid**";

    if(pdata->mode == UpdateMode::push) std::cout << " pushing";
    if(pdata->mode == UpdateMode::poll) std::cout << " polling";

    std::cout << " data type: " << pdata->valueType->name();
    std::cout << " length: " << pdata->nElements;

    std::cout << std::endl;
}

  /*********************************************************************************************************************/

  void VariableNetworkNode::createXML(xmlpp::Element *rootElement) const {
    if(pdata->type != NodeType::ControlSystem) return;

    // Create the directory for the path name in the XML document with all parent directories, if not yet existing:
    // First split the publication name into components and loop over each component. For each component, try to find
    // the directory node and create it it does not exist. After the loop, the "current" will point to the Element
    // representing the directory.

    // strip the variable name from the path
    mtca4u::RegisterPath directory(pdata->publicName);
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
    mtca4u::RegisterPath pathName(pdata->publicName);
    auto pathComponents = pathName.getComponents();

    // set the name attribute
    variable->set_attribute("name",pathComponents[pathComponents.size()-1]);

    // add sub-element containing the data type
    std::string dataTypeName{"unknown"};
    if(pdata->network->getValueType() == typeid(int8_t)) { dataTypeName = "int8"; }
    else if(pdata->network->getValueType() == typeid(uint8_t)) { dataTypeName = "uint8"; }
    else if(pdata->network->getValueType() == typeid(int16_t)) { dataTypeName = "int16"; }
    else if(pdata->network->getValueType() == typeid(uint16_t)) { dataTypeName = "uint16"; }
    else if(pdata->network->getValueType() == typeid(int32_t)) { dataTypeName = "int32"; }
    else if(pdata->network->getValueType() == typeid(uint32_t)) { dataTypeName = "uint32"; }
    else if(pdata->network->getValueType() == typeid(float)) { dataTypeName = "float"; }
    else if(pdata->network->getValueType() == typeid(double)) { dataTypeName = "double"; }
    else if(pdata->network->getValueType() == typeid(std::string)) { dataTypeName = "string"; }
    xmlpp::Element *valueTypeElement = variable->add_child("value_type");
    valueTypeElement->set_child_text(dataTypeName);

    // add sub-element containing the data flow direction
    std::string dataFlowName{"application_to_control_system"};
    if(pdata->network->getFeedingNode() == *this) { dataFlowName = "control_system_to_application"; }
    xmlpp::Element *directionElement = variable->add_child("direction");
    directionElement->set_child_text(dataFlowName);

    // add sub-element containing the engineering unit
    xmlpp::Element *unitElement = variable->add_child("unit");
    unitElement->set_child_text(pdata->network->getUnit());

    // add sub-element containing the description
    xmlpp::Element *descriptionElement = variable->add_child("description");
    descriptionElement->set_child_text(pdata->network->getDescription());
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::operator==(const VariableNetworkNode& other) const {
    if(other.pdata != pdata) return false;
    return true;
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::operator!=(const VariableNetworkNode& other) const {
    return !operator==(other);
  }

  /*********************************************************************************************************************/
/*
  VariableNetworkNode& VariableNetworkNode::operator<<(const VariableNetworkNode &other) {
    if(pdata->direction == VariableDirection::invalid) pdata->direction = VariableDirection::consuming;
    if(other.pdata->direction == VariableDirection::invalid) other.pdata->direction = VariableDirection::feeding;
    Application::getInstance().connect(*this, other);
    return *this;
  }
*/
  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetworkNode::operator>>(VariableNetworkNode other) {
    if(pdata->direction == VariableDirection::invalid) pdata->direction = VariableDirection::feeding;
    if(other.pdata->direction == VariableDirection::invalid) other.pdata->direction = VariableDirection::consuming;
    Application::getInstance().connect(*this, other);
    return *this;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetworkNode::operator[](VariableNetworkNode trigger) {

    // check if node already has a trigger
    if(pdata->externalTrigger != nullptr) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Only one external trigger per variable network is allowed.");
    }

    // force direction of the node we are operating on to be feeding
    if(pdata->direction == VariableDirection::invalid) pdata->direction = VariableDirection::feeding;
    assert(pdata->direction == VariableDirection::feeding);

    // force direction of the triggering node to be feeding
    if(trigger.pdata->direction == VariableDirection::invalid) trigger.pdata->direction = VariableDirection::feeding;
    assert(trigger.pdata->direction == VariableDirection::feeding);

    // create copy of the node
    VariableNetworkNode nodeWithTrigger;
    nodeWithTrigger.pdata.reset(new data(*pdata));

    // add ourselves as a trigger receiver to the other network
    if(!trigger.hasOwner()) {
      Application::getInstance().createNetwork().addNode(trigger);
    }
    trigger.getOwner().addTriggerReceiver(nodeWithTrigger);

    // set flag and store pointer to other network
    nodeWithTrigger.pdata->externalTrigger = &trigger;

    // return the new node
    return nodeWithTrigger;
  }

}
