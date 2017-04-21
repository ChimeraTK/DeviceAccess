/*
 * VariableNetworkNode.cc
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#include <libxml++/libxml++.h>

#include "VariableNetworkNode.h"
#include "VariableNetwork.h"
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

  VariableNetworkNode::VariableNetworkNode(mtca4u::TransferElement *accessorBridge, const std::string &name,
                                  VariableDirection direction, std::string unit, size_t nElements, UpdateMode mode,
                                  const std::string &description, const std::type_info* valueType,
                                  const std::unordered_set<std::string> &tags)
  : pdata(new VariableNetworkNode_data)
  {
    pdata->type = NodeType::Application;
    pdata->appNode = accessorBridge;
    pdata->name = name;
    pdata->mode = mode;
    pdata->direction = direction;
    pdata->valueType = valueType;
    pdata->unit = unit;
    pdata->nElements = nElements;
    pdata->description = description;
    pdata->tags = tags;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(const std::string &devAlias, const std::string &regName, UpdateMode mod,
      VariableDirection dir, const std::type_info &valTyp, size_t nElements)
  : pdata(new VariableNetworkNode_data)
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
  : pdata(new VariableNetworkNode_data)
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
  : pdata(new VariableNetworkNode_data)
  {
    pdata->type = NodeType::TriggerReceiver;
    pdata->direction = VariableDirection::consuming;
    pdata->nodeToTrigger = nodeToTrigger;
    pdata->name = "trigger:"+nodeToTrigger.getName();
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setOwner(VariableNetwork *net) {
    assert(pdata->network == nullptr);
    assert(pdata->type != NodeType::invalid);
    pdata->network = net;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::clearOwner() {
    pdata->network = nullptr;
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::hasImplementation() const {
    return pdata->type == NodeType::Device || pdata->type == NodeType::ControlSystem || pdata->type == NodeType::Constant;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::dump(std::ostream& stream) const {
    if(pdata->type == NodeType::Application) stream << " type = Application ('" << pdata->name << "')";
    if(pdata->type == NodeType::ControlSystem) stream << " type = ControlSystem ('" << pdata->publicName << "')";
    if(pdata->type == NodeType::Device) stream << " type = Device (" << pdata->deviceAlias << ": " << pdata->registerName << ")";
    if(pdata->type == NodeType::TriggerReceiver) stream << " type = TriggerReceiver";
    if(pdata->type == NodeType::Constant) stream << " type = Constant";
    if(pdata->type == NodeType::invalid) stream << " type = **invalid**";

    if(pdata->mode == UpdateMode::push) stream << " pushing";
    if(pdata->mode == UpdateMode::poll) stream << " polling";

    stream << " data type: " << pdata->valueType->name();
    stream << " length: " << pdata->nElements;

    stream << " [ptr: " << &(*pdata) << "]";
    
    stream << " { ";
    for(auto &tag : pdata->tags) stream << tag << " ";
    stream << "}";
    
    stream << std::endl;
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

    // add sub-element containing the description
    xmlpp::Element *nElementsElement = variable->add_child("numberOfElements");
    nElementsElement->set_child_text(std::to_string(pdata->network->getFeedingNode().getNumberOfElements()));
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::operator==(const VariableNetworkNode& other) const {
    return (other.pdata == pdata) || (pdata->type == NodeType::invalid && other.pdata->type == NodeType::invalid);
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::operator!=(const VariableNetworkNode& other) const {
    return !operator==(other);
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::operator<(const VariableNetworkNode& other) const {
    if(pdata->type == NodeType::invalid && other.pdata->type == NodeType::invalid) return false;
    return (other.pdata < pdata);
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
    if(pdata->externalTrigger.getType() != NodeType::invalid) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Only one external trigger per variable network is allowed.");
    }

    // force direction of the node we are operating on to be feeding
    if(pdata->direction == VariableDirection::invalid) pdata->direction = VariableDirection::feeding;
    assert(pdata->direction == VariableDirection::feeding);

    // force direction of the triggering node to be feeding
    if(trigger.pdata->direction == VariableDirection::invalid) trigger.pdata->direction = VariableDirection::feeding;
    assert(trigger.pdata->direction == VariableDirection::feeding);
    
    // check if already existing in map
    if(pdata->nodeWithTrigger.count(trigger) > 0) {
      return pdata->nodeWithTrigger[trigger];
    }

    // create copy of the node
    pdata->nodeWithTrigger[trigger].pdata.reset(new VariableNetworkNode_data(*pdata));

    // add ourselves as a trigger receiver to the other network
    if(!trigger.hasOwner()) {
      Application::getInstance().createNetwork().addNode(trigger);
    }
    trigger.getOwner().addNodeToTrigger(pdata->nodeWithTrigger[trigger]);

    // set flag and store pointer to other network
    pdata->nodeWithTrigger[trigger].pdata->externalTrigger = trigger;

    // return the new node
    return pdata->nodeWithTrigger[trigger];
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode() {}

  /*********************************************************************************************************************/
  
  void VariableNetworkNode::setValueType(const std::type_info& newType) const {
    assert(*pdata->valueType == typeid(AnyType));
    pdata->valueType = &newType;
  }
  
  /*********************************************************************************************************************/

  bool VariableNetworkNode::hasExternalTrigger() const {
    return pdata->externalTrigger.getType() != NodeType::invalid;
  }
      
  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetworkNode::getExternalTrigger() {
    assert(pdata->externalTrigger.getType() != NodeType::invalid);
    return pdata->externalTrigger;
  }

  /*********************************************************************************************************************/

  bool VariableNetworkNode::hasOwner() const {
    return pdata->network != nullptr;
  }

  /*********************************************************************************************************************/

  NodeType VariableNetworkNode::getType() const {
    if(!pdata) return NodeType::invalid;
    return pdata->type;
  }

  /*********************************************************************************************************************/

  UpdateMode VariableNetworkNode::getMode() const {
    return pdata->mode;
  }

  /*********************************************************************************************************************/

  VariableDirection VariableNetworkNode::getDirection() const {
    return pdata->direction;
  }

  /*********************************************************************************************************************/

  const std::type_info& VariableNetworkNode::getValueType() const {
    return *(pdata->valueType);
  }

  /*********************************************************************************************************************/

  std::string VariableNetworkNode::getName() const {
    if(pdata->type == NodeType::ControlSystem) {
      return pdata->publicName;
    }
    else if(pdata->type == NodeType::Device) {
      return pdata->deviceAlias+":"+pdata->registerName;
    }
    else {
      return pdata->name;
    }
  }

  /*********************************************************************************************************************/

  const std::string& VariableNetworkNode::getUnit() const {
    return pdata->unit;
  }

  /*********************************************************************************************************************/

  const std::string& VariableNetworkNode::getDescription() const {
    return pdata->description;
  }

  /*********************************************************************************************************************/

  VariableNetwork& VariableNetworkNode::getOwner() const {
    assert(pdata->network != nullptr);
    return *(pdata->network);
  }

  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetworkNode::getNodeToTrigger() {
    assert(pdata->nodeToTrigger.getType() != NodeType::invalid);
    return pdata->nodeToTrigger;
  }

  /*********************************************************************************************************************/

  const std::string& VariableNetworkNode::getPublicName() const {
    assert(pdata->type == NodeType::ControlSystem);
    return pdata->publicName;
  }

  /*********************************************************************************************************************/

  const std::string& VariableNetworkNode::getDeviceAlias() const {
    assert(pdata->type == NodeType::Device);
    return pdata->deviceAlias;
  }

  /*********************************************************************************************************************/

  const std::string& VariableNetworkNode::getRegisterName() const {
    assert(pdata->type == NodeType::Device);
    return pdata->registerName;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setNumberOfElements(size_t nElements) {
    pdata->nElements = nElements;
  }

  /*********************************************************************************************************************/

  size_t VariableNetworkNode::getNumberOfElements() const {
    return pdata->nElements;
  }

  /*********************************************************************************************************************/

  mtca4u::TransferElement& VariableNetworkNode::getAppAccessorNoType() {
    return *(pdata->appNode);
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setMetaData(const std::string &name, const std::string &unit,
                                        const std::string &description) {
    if(getType() != NodeType::Application) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
          "Calling VariableNetworkNode::updateMetaData() is not allowed for non-application type nodes.");
    }
    pdata->name = name;
    pdata->unit = unit;
    pdata->description = description;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setMetaData(const std::string &name, const std::string &unit,
                                        const std::string &description, const std::unordered_set<std::string> &tags) {
    setMetaData(name, unit, description);
    pdata->tags = tags;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::addTag(const std::string &tag) {
    pdata->tags.insert(tag);
  }

  /*********************************************************************************************************************/
  
  const std::unordered_set<std::string>& VariableNetworkNode::getTags() const {
    return pdata->tags;
  }

}
