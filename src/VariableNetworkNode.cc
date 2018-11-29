/*
 * VariableNetworkNode.cc
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#include "VariableNetworkNode.h"
#include "VariableNetwork.h"
#include "Application.h"
#include "EntityOwner.h"
#include "Visitor.h"
#include "VariableNetworkNodeDumpingVisitor.h"

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

  VariableNetworkNode::VariableNetworkNode(EntityOwner *owner, ChimeraTK::TransferElementAbstractor *accessorBridge,
                                  const std::string &name,
                                  VariableDirection direction, std::string unit, size_t nElements, UpdateMode mode,
                                  const std::string &description, const std::type_info* valueType,
                                  const std::unordered_set<std::string> &tags)
  : pdata(boost::make_shared<VariableNetworkNode_data>())
  {
    pdata->owningModule = owner;
    pdata->type = NodeType::Application;
    pdata->appNode = accessorBridge;
    pdata->name = name;
    pdata->qualifiedName = owner->getQualifiedName() + "/" + name;
    pdata->mode = mode;
    pdata->direction = direction;
    pdata->valueType = valueType;
    pdata->unit = unit;
    pdata->nElements = nElements;
    pdata->description = description;
    pdata->tags = tags;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(const std::string &devAlias, const std::string &regName, UpdateMode mode,
      VariableDirection dir, const std::type_info &valTyp, size_t nElements)
  : pdata(boost::make_shared<VariableNetworkNode_data>())
  {
    pdata->type = NodeType::Device;
    pdata->mode = mode;
    pdata->direction = dir;
    pdata->valueType = &valTyp;
    pdata->deviceAlias = devAlias;
    pdata->registerName = regName;
    pdata->nElements = nElements;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(std::string pubName, VariableDirection dir, const std::type_info &valTyp,
      size_t nElements)
  : pdata(boost::make_shared<VariableNetworkNode_data>())
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
  : pdata(boost::make_shared<VariableNetworkNode_data>())
  {
    pdata->type = NodeType::TriggerReceiver;
    pdata->direction = VariableDirection::consuming;
    pdata->nodeToTrigger = nodeToTrigger;
    pdata->name = "trigger:"+nodeToTrigger.getName();
  }

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode(boost::shared_ptr<VariableNetworkNode_data> _pdata)
  : pdata(_pdata)
  {}

  /*********************************************************************************************************************/

  VariableNetworkNode::VariableNetworkNode()
  : pdata(boost::make_shared<VariableNetworkNode_data>())
  {}

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

  void VariableNetworkNode::accept( Visitor<VariableNetworkNode>& visitor) const {
      visitor.dispatch(*this);
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

  VariableNetworkNode VariableNetworkNode::operator>>(VariableNetworkNode other) {
    if(pdata->direction == VariableDirection::invalid) {
      if(!other.hasOwner()) {
        pdata->direction = VariableDirection::feeding;
      }
      else {
        if(other.getOwner().hasFeedingNode()) {
          pdata->direction = VariableDirection::consuming;
          if(getType() == NodeType::Device) {   // special treatment for Device-type variables: consumers are push-type
            pdata->mode = UpdateMode::push;
          }
        }
        else {
          pdata->direction = VariableDirection::feeding;
        }
      }
    }
    if(other.pdata->direction == VariableDirection::invalid) {
      if(!hasOwner()) {
        other.pdata->direction = VariableDirection::consuming;
        if(other.getType() == NodeType::Device) {   // special treatment for Device-type variables: consumers are push-type
          other.pdata->mode = UpdateMode::push;
        }
      }
      else {
        if(getOwner().hasFeedingNode()) {
          other.pdata->direction = VariableDirection::consuming;
          if(other.getType() == NodeType::Device) {   // special treatment for Device-type variables: consumers are push-type
            other.pdata->mode = UpdateMode::push;
          }
        }
        else {
          other.pdata->direction = VariableDirection::feeding;
        }
      }
    }
    Application::getInstance().connect(*this, other);
    return *this;
  }

  /*********************************************************************************************************************/

  VariableNetworkNode VariableNetworkNode::operator[](VariableNetworkNode trigger) {

    // check if node already has a trigger
    if(pdata->externalTrigger.getType() != NodeType::invalid) {
      throw ChimeraTK::logic_error("Only one external trigger per variable network is allowed.");
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
    pdata->nodeWithTrigger[trigger].pdata = boost::make_shared<VariableNetworkNode_data>(*pdata);

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

  void VariableNetworkNode::dump(std::ostream& stream) const {
      VariableNetworkNodeDumpingVisitor visitor(stream, " ");
      visitor.dispatch(*this);
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
      return pdata->registerName;
    }
    else {
      return pdata->name;
    }
  }

  /*********************************************************************************************************************/

  std::string VariableNetworkNode::getQualifiedName() const {
    return pdata->qualifiedName;
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

  ChimeraTK::TransferElementAbstractor& VariableNetworkNode::getAppAccessorNoType() {
    return *(pdata->appNode);
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setMetaData(const std::string &name, const std::string &unit,
                                        const std::string &description) {
    if(getType() != NodeType::Application) {
      throw ChimeraTK::logic_error("Calling VariableNetworkNode::updateMetaData() is not allowed for non-application type nodes.");
    }
    pdata->name = name;
    pdata->qualifiedName = pdata->owningModule->getQualifiedName()+"/"+name;
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

  /*********************************************************************************************************************/

  void VariableNetworkNode::setAppAccessorPointer(ChimeraTK::TransferElementAbstractor *accessor) {
    assert(getType() == NodeType::Application);
    pdata->appNode = accessor;
  }

  /*********************************************************************************************************************/

  EntityOwner* VariableNetworkNode::getOwningModule() const {
    return pdata->owningModule;
  }

  /*********************************************************************************************************************/

  void VariableNetworkNode::setOwningModule(EntityOwner *newOwner) const {
    pdata->owningModule = newOwner;
  }

}
